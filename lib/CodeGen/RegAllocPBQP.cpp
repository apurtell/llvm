//===------ RegAllocPBQP.cpp ---- PBQP Register Allocator -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a Partitioned Boolean Quadratic Programming (PBQP) based
// register allocator for LLVM. This allocator works by constructing a PBQP
// problem representing the register allocation problem under consideration,
// solving this using a PBQP solver, and mapping the solution back to a
// register assignment. If any variables are selected for spilling then spill
// code is inserted and the process repeated.
//
// The PBQP solver (pbqp.c) provided for this allocator uses a heuristic tuned
// for register allocation. For more information on PBQP for register
// allocation, see the following papers:
//
//   (1) Hames, L. and Scholz, B. 2006. Nearly optimal register allocation with
//   PBQP. In Proceedings of the 7th Joint Modular Languages Conference
//   (JMLC'06). LNCS, vol. 4228. Springer, New York, NY, USA. 346-361.
//
//   (2) Scholz, B., Eckstein, E. 2002. Register allocation for irregular
//   architectures. In Proceedings of the Joint Conference on Languages,
//   Compilers and Tools for Embedded Systems (LCTES'02), ACM Press, New York,
//   NY, USA, 139-148.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/RegAllocPBQP.h"
#include "RegisterCoalescer.h"
#include "Spiller.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/CodeGen/CalcSpillWeights.h"
#include "llvm/CodeGen/LiveIntervalAnalysis.h"
#include "llvm/CodeGen/LiveRangeEdit.h"
#include "llvm/CodeGen/LiveStackAnalysis.h"
#include "llvm/CodeGen/MachineBlockFrequencyInfo.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegAllocRegistry.h"
#include "llvm/CodeGen/VirtRegMap.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "regalloc"

static RegisterRegAlloc
RegisterPBQPRepAlloc("pbqp", "PBQP register allocator",
                       createDefaultPBQPRegisterAllocator);

static cl::opt<bool>
PBQPCoalescing("pbqp-coalescing",
                cl::desc("Attempt coalescing during PBQP register allocation."),
                cl::init(false), cl::Hidden);

#ifndef NDEBUG
static cl::opt<bool>
PBQPDumpGraphs("pbqp-dump-graphs",
               cl::desc("Dump graphs for each function/round in the compilation unit."),
               cl::init(false), cl::Hidden);
#endif

namespace {

///
/// PBQP based allocators solve the register allocation problem by mapping
/// register allocation problems to Partitioned Boolean Quadratic
/// Programming problems.
class RegAllocPBQP : public MachineFunctionPass {
public:

  static char ID;

  /// Construct a PBQP register allocator.
  RegAllocPBQP(char *cPassID = nullptr)
      : MachineFunctionPass(ID), customPassID(cPassID) {
    initializeSlotIndexesPass(*PassRegistry::getPassRegistry());
    initializeLiveIntervalsPass(*PassRegistry::getPassRegistry());
    initializeLiveStacksPass(*PassRegistry::getPassRegistry());
    initializeVirtRegMapPass(*PassRegistry::getPassRegistry());
  }

  /// Return the pass name.
  const char* getPassName() const override {
    return "PBQP Register Allocator";
  }

  /// PBQP analysis usage.
  void getAnalysisUsage(AnalysisUsage &au) const override;

  /// Perform register allocation
  bool runOnMachineFunction(MachineFunction &MF) override;

private:

  typedef std::map<const LiveInterval*, unsigned> LI2NodeMap;
  typedef std::vector<const LiveInterval*> Node2LIMap;
  typedef std::vector<unsigned> AllowedSet;
  typedef std::vector<AllowedSet> AllowedSetMap;
  typedef std::pair<unsigned, unsigned> RegPair;
  typedef std::map<RegPair, PBQP::PBQPNum> CoalesceMap;
  typedef std::set<unsigned> RegSet;

  char *customPassID;

  RegSet VRegsToAlloc, EmptyIntervalVRegs;

  /// \brief Finds the initial set of vreg intervals to allocate.
  void findVRegIntervalsToAlloc(const MachineFunction &MF, LiveIntervals &LIS);

  /// \brief Constructs an initial graph.
  void initializeGraph(PBQPRAGraph &G);

  /// \brief Given a solved PBQP problem maps this solution back to a register
  /// assignment.
  bool mapPBQPToRegAlloc(const PBQPRAGraph &G,
                         const PBQP::Solution &Solution,
                         VirtRegMap &VRM,
                         Spiller &VRegSpiller);

  /// \brief Postprocessing before final spilling. Sets basic block "live in"
  /// variables.
  void finalizeAlloc(MachineFunction &MF, LiveIntervals &LIS,
                     VirtRegMap &VRM) const;

};

char RegAllocPBQP::ID = 0;

/// @brief Set spill costs for each node in the PBQP reg-alloc graph.
class SpillCosts : public PBQPRAConstraint {
public:
  void apply(PBQPRAGraph &G) override {
    LiveIntervals &LIS = G.getMetadata().LIS;

    for (auto NId : G.nodeIds()) {
      PBQP::PBQPNum SpillCost =
        LIS.getInterval(G.getNodeMetadata(NId).getVReg()).weight;
      if (SpillCost == 0.0)
        SpillCost = std::numeric_limits<PBQP::PBQPNum>::min();
      PBQPRAGraph::RawVector NodeCosts(G.getNodeCosts(NId));
      NodeCosts[PBQP::RegAlloc::getSpillOptionIdx()] = SpillCost;
      G.setNodeCosts(NId, std::move(NodeCosts));
    }
  }
};

/// @brief Add interference edges between overlapping vregs.
class Interference : public PBQPRAConstraint {
public:

  void apply(PBQPRAGraph &G) override {
    LiveIntervals &LIS = G.getMetadata().LIS;
    const TargetRegisterInfo &TRI =
      *G.getMetadata().MF.getTarget().getSubtargetImpl()->getRegisterInfo();

    for (auto NItr = G.nodeIds().begin(), NEnd = G.nodeIds().end();
         NItr != NEnd; ++NItr) {
      auto NId = *NItr;
      unsigned NVReg = G.getNodeMetadata(NId).getVReg();
      LiveInterval &NLI = LIS.getInterval(NVReg);

      for (auto MItr = std::next(NItr); MItr != NEnd; ++MItr) {
        auto MId = *MItr;
        unsigned MVReg = G.getNodeMetadata(MId).getVReg();
        LiveInterval &MLI = LIS.getInterval(MVReg);

        if (NLI.overlaps(MLI)) {
          const auto &NOpts = G.getNodeMetadata(NId).getOptionRegs();
          const auto &MOpts = G.getNodeMetadata(MId).getOptionRegs();
          G.addEdge(NId, MId, createInterferenceMatrix(TRI, NOpts, MOpts));
        }
      }
    }
  }

private:

  PBQPRAGraph::RawMatrix createInterferenceMatrix(
                       const TargetRegisterInfo &TRI,
                       const PBQPRAGraph::NodeMetadata::OptionToRegMap &NOpts,
                       const PBQPRAGraph::NodeMetadata::OptionToRegMap &MOpts) {
    PBQPRAGraph::RawMatrix M(NOpts.size() + 1, MOpts.size() + 1, 0);
    for (unsigned I = 0; I != NOpts.size(); ++I) {
      unsigned PRegN = NOpts[I];
      for (unsigned J = 0; J != MOpts.size(); ++J) {
        unsigned PRegM = MOpts[J];
        if (TRI.regsOverlap(PRegN, PRegM))
          M[I + 1][J + 1] = std::numeric_limits<PBQP::PBQPNum>::infinity();
      }
    }

    return M;
  }
};


class Coalescing : public PBQPRAConstraint {
public:
  void apply(PBQPRAGraph &G) override {
    MachineFunction &MF = G.getMetadata().MF;
    MachineBlockFrequencyInfo &MBFI = G.getMetadata().MBFI;
    CoalescerPair CP(*MF.getTarget().getSubtargetImpl()->getRegisterInfo());

    // Scan the machine function and add a coalescing cost whenever CoalescerPair
    // gives the Ok.
    for (const auto &MBB : MF) {
      for (const auto &MI : MBB) {

        // Skip not-coalescable or already coalesced copies.
        if (!CP.setRegisters(&MI) || CP.getSrcReg() == CP.getDstReg())
          continue;

        unsigned DstReg = CP.getDstReg();
        unsigned SrcReg = CP.getSrcReg();

        const float CopyFactor = 0.5; // Cost of copy relative to load. Current
                                      // value plucked randomly out of the air.

        PBQP::PBQPNum CBenefit =
          CopyFactor * LiveIntervals::getSpillWeight(false, true, &MBFI, &MI);

        if (CP.isPhys()) {
          if (!MF.getRegInfo().isAllocatable(DstReg))
            continue;

          PBQPRAGraph::NodeId NId = G.getMetadata().getNodeIdForVReg(SrcReg);

          const PBQPRAGraph::NodeMetadata::OptionToRegMap &Allowed =
            G.getNodeMetadata(NId).getOptionRegs();

          unsigned PRegOpt = 0;
          while (PRegOpt < Allowed.size() && Allowed[PRegOpt] != DstReg)
            ++PRegOpt;

          if (PRegOpt < Allowed.size()) {
            PBQPRAGraph::RawVector NewCosts(G.getNodeCosts(NId));
            NewCosts[PRegOpt + 1] += CBenefit;
            G.setNodeCosts(NId, std::move(NewCosts));
          }
        } else {
          PBQPRAGraph::NodeId N1Id = G.getMetadata().getNodeIdForVReg(DstReg);
          PBQPRAGraph::NodeId N2Id = G.getMetadata().getNodeIdForVReg(SrcReg);
          const PBQPRAGraph::NodeMetadata::OptionToRegMap *Allowed1 =
            &G.getNodeMetadata(N1Id).getOptionRegs();
          const PBQPRAGraph::NodeMetadata::OptionToRegMap *Allowed2 =
            &G.getNodeMetadata(N2Id).getOptionRegs();

          PBQPRAGraph::EdgeId EId = G.findEdge(N1Id, N2Id);
          if (EId == G.invalidEdgeId()) {
            PBQPRAGraph::RawMatrix Costs(Allowed1->size() + 1,
                                         Allowed2->size() + 1, 0);
            addVirtRegCoalesce(Costs, *Allowed1, *Allowed2, CBenefit);
            G.addEdge(N1Id, N2Id, std::move(Costs));
          } else {
            if (G.getEdgeNode1Id(EId) == N2Id) {
              std::swap(N1Id, N2Id);
              std::swap(Allowed1, Allowed2);
            }
            PBQPRAGraph::RawMatrix Costs(G.getEdgeCosts(EId));
            addVirtRegCoalesce(Costs, *Allowed1, *Allowed2, CBenefit);
            G.setEdgeCosts(EId, std::move(Costs));
          }
        }
      }
    }
  }

private:

  void addVirtRegCoalesce(
                      PBQPRAGraph::RawMatrix &CostMat,
                      const PBQPRAGraph::NodeMetadata::OptionToRegMap &Allowed1,
                      const PBQPRAGraph::NodeMetadata::OptionToRegMap &Allowed2,
                      PBQP::PBQPNum Benefit) {
    assert(CostMat.getRows() == Allowed1.size() + 1 && "Size mismatch.");
    assert(CostMat.getCols() == Allowed2.size() + 1 && "Size mismatch.");
    for (unsigned I = 0; I != Allowed1.size(); ++I) {
      unsigned PReg1 = Allowed1[I];
      for (unsigned J = 0; J != Allowed2.size(); ++J) {
        unsigned PReg2 = Allowed2[J];
        if (PReg1 == PReg2)
          CostMat[I + 1][J + 1] += -Benefit;
      }
    }
  }

};

} // End anonymous namespace.

// Out-of-line destructor/anchor for PBQPRAConstraint.
PBQPRAConstraint::~PBQPRAConstraint() {}
void PBQPRAConstraint::anchor() {}
void PBQPRAConstraintList::anchor() {}

void RegAllocPBQP::getAnalysisUsage(AnalysisUsage &au) const {
  au.setPreservesCFG();
  au.addRequired<AliasAnalysis>();
  au.addPreserved<AliasAnalysis>();
  au.addRequired<SlotIndexes>();
  au.addPreserved<SlotIndexes>();
  au.addRequired<LiveIntervals>();
  au.addPreserved<LiveIntervals>();
  //au.addRequiredID(SplitCriticalEdgesID);
  if (customPassID)
    au.addRequiredID(*customPassID);
  au.addRequired<LiveStacks>();
  au.addPreserved<LiveStacks>();
  au.addRequired<MachineBlockFrequencyInfo>();
  au.addPreserved<MachineBlockFrequencyInfo>();
  au.addRequired<MachineLoopInfo>();
  au.addPreserved<MachineLoopInfo>();
  au.addRequired<MachineDominatorTree>();
  au.addPreserved<MachineDominatorTree>();
  au.addRequired<VirtRegMap>();
  au.addPreserved<VirtRegMap>();
  MachineFunctionPass::getAnalysisUsage(au);
}

void RegAllocPBQP::findVRegIntervalsToAlloc(const MachineFunction &MF,
                                            LiveIntervals &LIS) {
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  // Iterate over all live ranges.
  for (unsigned I = 0, E = MRI.getNumVirtRegs(); I != E; ++I) {
    unsigned Reg = TargetRegisterInfo::index2VirtReg(I);
    if (MRI.reg_nodbg_empty(Reg))
      continue;
    LiveInterval &LI = LIS.getInterval(Reg);

    // If this live interval is non-empty we will use pbqp to allocate it.
    // Empty intervals we allocate in a simple post-processing stage in
    // finalizeAlloc.
    if (!LI.empty()) {
      VRegsToAlloc.insert(LI.reg);
    } else {
      EmptyIntervalVRegs.insert(LI.reg);
    }
  }
}

void RegAllocPBQP::initializeGraph(PBQPRAGraph &G) {
  MachineFunction &MF = G.getMetadata().MF;

  LiveIntervals &LIS = G.getMetadata().LIS;
  const MachineRegisterInfo &MRI = G.getMetadata().MF.getRegInfo();
  const TargetRegisterInfo &TRI =
    *G.getMetadata().MF.getTarget().getSubtargetImpl()->getRegisterInfo();

  for (auto VReg : VRegsToAlloc) {
    const TargetRegisterClass *TRC = MRI.getRegClass(VReg);
    LiveInterval &VRegLI = LIS.getInterval(VReg);

    // Record any overlaps with regmask operands.
    BitVector RegMaskOverlaps;
    LIS.checkRegMaskInterference(VRegLI, RegMaskOverlaps);

    // Compute an initial allowed set for the current vreg.
    std::vector<unsigned> VRegAllowed;
    ArrayRef<MCPhysReg> RawPRegOrder = TRC->getRawAllocationOrder(MF);
    for (unsigned I = 0; I != RawPRegOrder.size(); ++I) {
      unsigned PReg = RawPRegOrder[I];
      if (MRI.isReserved(PReg))
        continue;

      // vregLI crosses a regmask operand that clobbers preg.
      if (!RegMaskOverlaps.empty() && !RegMaskOverlaps.test(PReg))
        continue;

      // vregLI overlaps fixed regunit interference.
      bool Interference = false;
      for (MCRegUnitIterator Units(PReg, &TRI); Units.isValid(); ++Units) {
        if (VRegLI.overlaps(LIS.getRegUnit(*Units))) {
          Interference = true;
          break;
        }
      }
      if (Interference)
        continue;

      // preg is usable for this virtual register.
      VRegAllowed.push_back(PReg);
    }

    PBQPRAGraph::RawVector NodeCosts(VRegAllowed.size() + 1, 0);
    PBQPRAGraph::NodeId NId = G.addNode(std::move(NodeCosts));
    G.getNodeMetadata(NId).setVReg(VReg);
    G.getNodeMetadata(NId).setOptionRegs(std::move(VRegAllowed));
    G.getMetadata().setNodeIdForVReg(VReg, NId);
  }
}

bool RegAllocPBQP::mapPBQPToRegAlloc(const PBQPRAGraph &G,
                                     const PBQP::Solution &Solution,
                                     VirtRegMap &VRM,
                                     Spiller &VRegSpiller) {
  MachineFunction &MF = G.getMetadata().MF;
  LiveIntervals &LIS = G.getMetadata().LIS;
  const TargetRegisterInfo &TRI =
    *MF.getTarget().getSubtargetImpl()->getRegisterInfo();
  (void)TRI;

  // Set to true if we have any spills
  bool AnotherRoundNeeded = false;

  // Clear the existing allocation.
  VRM.clearAllVirt();

  // Iterate over the nodes mapping the PBQP solution to a register
  // assignment.
  for (auto NId : G.nodeIds()) {
    unsigned VReg = G.getNodeMetadata(NId).getVReg();
    unsigned AllocOption = Solution.getSelection(NId);

    if (AllocOption != PBQP::RegAlloc::getSpillOptionIdx()) {
      unsigned PReg = G.getNodeMetadata(NId).getOptionRegs()[AllocOption - 1];
      DEBUG(dbgs() << "VREG " << PrintReg(VReg, &TRI) << " -> "
            << TRI.getName(PReg) << "\n");
      assert(PReg != 0 && "Invalid preg selected.");
      VRM.assignVirt2Phys(VReg, PReg);
    } else {
      VRegsToAlloc.erase(VReg);
      SmallVector<unsigned, 8> NewSpills;
      LiveRangeEdit LRE(&LIS.getInterval(VReg), NewSpills, MF, LIS, &VRM);
      VRegSpiller.spill(LRE);

      DEBUG(dbgs() << "VREG " << PrintReg(VReg, &TRI) << " -> SPILLED (Cost: "
                   << LRE.getParent().weight << ", New vregs: ");

      // Copy any newly inserted live intervals into the list of regs to
      // allocate.
      for (LiveRangeEdit::iterator I = LRE.begin(), E = LRE.end();
           I != E; ++I) {
        LiveInterval &LI = LIS.getInterval(*I);
        assert(!LI.empty() && "Empty spill range.");
        DEBUG(dbgs() << PrintReg(LI.reg, &TRI) << " ");
        VRegsToAlloc.insert(LI.reg);
      }

      DEBUG(dbgs() << ")\n");

      // We need another round if spill intervals were added.
      AnotherRoundNeeded |= !LRE.empty();
    }
  }

  return !AnotherRoundNeeded;
}


void RegAllocPBQP::finalizeAlloc(MachineFunction &MF,
                                 LiveIntervals &LIS,
                                 VirtRegMap &VRM) const {
  MachineRegisterInfo &MRI = MF.getRegInfo();

  // First allocate registers for the empty intervals.
  for (RegSet::const_iterator
         I = EmptyIntervalVRegs.begin(), E = EmptyIntervalVRegs.end();
         I != E; ++I) {
    LiveInterval &LI = LIS.getInterval(*I);

    unsigned PReg = MRI.getSimpleHint(LI.reg);

    if (PReg == 0) {
      const TargetRegisterClass &RC = *MRI.getRegClass(LI.reg);
      PReg = RC.getRawAllocationOrder(MF).front();
    }

    VRM.assignVirt2Phys(LI.reg, PReg);
  }
}

bool RegAllocPBQP::runOnMachineFunction(MachineFunction &MF) {
  LiveIntervals &LIS = getAnalysis<LiveIntervals>();
  MachineBlockFrequencyInfo &MBFI =
    getAnalysis<MachineBlockFrequencyInfo>();

  calculateSpillWeightsAndHints(LIS, MF, getAnalysis<MachineLoopInfo>(), MBFI);

  VirtRegMap &VRM = getAnalysis<VirtRegMap>();

  std::unique_ptr<Spiller> VRegSpiller(createInlineSpiller(*this, MF, VRM));

  MF.getRegInfo().freezeReservedRegs(MF);

  DEBUG(dbgs() << "PBQP Register Allocating for " << MF.getName() << "\n");

  // Allocator main loop:
  //
  // * Map current regalloc problem to a PBQP problem
  // * Solve the PBQP problem
  // * Map the solution back to a register allocation
  // * Spill if necessary
  //
  // This process is continued till no more spills are generated.

  // Find the vreg intervals in need of allocation.
  findVRegIntervalsToAlloc(MF, LIS);

#ifndef NDEBUG
  const Function &F = *MF.getFunction();
  std::string FullyQualifiedName =
    F.getParent()->getModuleIdentifier() + "." + F.getName().str();
#endif

  // If there are non-empty intervals allocate them using pbqp.
  if (!VRegsToAlloc.empty()) {

    const TargetSubtargetInfo &Subtarget = *MF.getTarget().getSubtargetImpl();
    std::unique_ptr<PBQPRAConstraintList> ConstraintsRoot =
      llvm::make_unique<PBQPRAConstraintList>();
    ConstraintsRoot->addConstraint(llvm::make_unique<SpillCosts>());
    ConstraintsRoot->addConstraint(llvm::make_unique<Interference>());
    if (PBQPCoalescing)
      ConstraintsRoot->addConstraint(llvm::make_unique<Coalescing>());
    ConstraintsRoot->addConstraint(Subtarget.getCustomPBQPConstraints());

    bool PBQPAllocComplete = false;
    unsigned Round = 0;

    while (!PBQPAllocComplete) {
      DEBUG(dbgs() << "  PBQP Regalloc round " << Round << ":\n");

      PBQPRAGraph G(PBQPRAGraph::GraphMetadata(MF, LIS, MBFI));
      initializeGraph(G);
      ConstraintsRoot->apply(G);

#ifndef NDEBUG
      if (PBQPDumpGraphs) {
        std::ostringstream RS;
        RS << Round;
        std::string GraphFileName = FullyQualifiedName + "." + RS.str() +
                                    ".pbqpgraph";
        std::error_code EC;
        raw_fd_ostream OS(GraphFileName, EC, sys::fs::F_Text);
        DEBUG(dbgs() << "Dumping graph for round " << Round << " to \""
              << GraphFileName << "\"\n");
        G.dumpToStream(OS);
      }
#endif

      PBQP::Solution Solution = PBQP::RegAlloc::solve(G);
      PBQPAllocComplete = mapPBQPToRegAlloc(G, Solution, VRM, *VRegSpiller);
      ++Round;
    }
  }

  // Finalise allocation, allocate empty ranges.
  finalizeAlloc(MF, LIS, VRM);
  VRegsToAlloc.clear();
  EmptyIntervalVRegs.clear();

  DEBUG(dbgs() << "Post alloc VirtRegMap:\n" << VRM << "\n");

  return true;
}

FunctionPass *llvm::createPBQPRegisterAllocator(char *customPassID) {
  return new RegAllocPBQP(customPassID);
}

FunctionPass* llvm::createDefaultPBQPRegisterAllocator() {
  return createPBQPRegisterAllocator();
}

#undef DEBUG_TYPE
