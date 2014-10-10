//===-- SILoadStoreOptimizer.cpp ------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass tries to fuse DS instructions with close by immediate offsets.
// This will fuse operations such as
//  ds_read_b32 v0, v2 offset:16
//  ds_read_b32 v1, v2 offset:32
// ==>
//   ds_read2_b32 v[0:1], v2, offset0:4 offset1:8
//
//
// Future improvements:
//
// - This currently relies on the scheduler to place loads and stores next to
//   each other, and then only merges adjacent pairs of instructions. It would
//   be good to be more flexible with interleaved instructions, and possibly run
//   before scheduling. It currently missing stores of constants because loading
//   the constant into the data register is placed between the stores, although
//   this is arguably a scheduling problem.
//
// - Live interval recomputing seems inefficient. This currently only matches
//   one pair, and recomputes live intervals and moves on to the next pair. It
//   would be better to compute a list of all merges that need to occur
//
// - With a list of instructions to process, we can also merge more. If a
//   cluster of loads have offsets that are too large to fit in the 8-bit
//   offsets, but are close enough to fit in the 8 bits, we can add to the base
//   pointer and use the new reduced offsets.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "SIInstrInfo.h"
#include "SIRegisterInfo.h"
#include "llvm/CodeGen/LiveIntervalAnalysis.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "si-load-store-opt"

namespace {

class SILoadStoreOptimizer : public MachineFunctionPass {
private:
  const TargetMachine *TM;
  const SIInstrInfo *TII;
  const SIRegisterInfo *TRI;
  MachineRegisterInfo *MRI;
  LiveIntervals *LIS;


  static bool offsetsCanBeCombined(unsigned Offset0,
                                   unsigned Offset1,
                                   unsigned EltSize);

  MachineBasicBlock::iterator findMatchingDSInst(MachineBasicBlock::iterator I,
                                                 unsigned EltSize);

  void updateRegDefsUses(unsigned SrcReg,
                         unsigned DstReg,
                         unsigned SubIdx);

  MachineBasicBlock::iterator mergeRead2Pair(
    MachineBasicBlock::iterator I,
    MachineBasicBlock::iterator Paired,
    unsigned EltSize,
    const MCInstrDesc &Read2InstDesc);

  MachineBasicBlock::iterator mergeWrite2Pair(
    MachineBasicBlock::iterator I,
    MachineBasicBlock::iterator Paired,
    unsigned EltSize,
    const MCInstrDesc &Write2InstDesc);

public:
  static char ID;

  SILoadStoreOptimizer() :
    MachineFunctionPass(ID),
    TM(nullptr),
    TII(nullptr),
    TRI(nullptr),
    MRI(nullptr),
    LIS(nullptr) {

  }

  SILoadStoreOptimizer(const TargetMachine &TM_) :
    MachineFunctionPass(ID),
    TM(&TM_),
    TII(static_cast<const SIInstrInfo*>(TM->getSubtargetImpl()->getInstrInfo())) {
    initializeSILoadStoreOptimizerPass(*PassRegistry::getPassRegistry());
  }

  bool optimizeBlock(MachineBasicBlock &MBB);

  bool runOnMachineFunction(MachineFunction &MF) override;

  const char *getPassName() const override {
    return "SI Load / Store Optimizer";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreserved<SlotIndexes>();
    AU.addPreserved<LiveIntervals>();
    AU.addPreserved<LiveVariables>();
    AU.addRequired<LiveIntervals>();

    MachineFunctionPass::getAnalysisUsage(AU);
  }
};

} // End anonymous namespace.

INITIALIZE_PASS_BEGIN(SILoadStoreOptimizer, DEBUG_TYPE,
                      "SI Load / Store Optimizer", false, false)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(LiveVariables)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_END(SILoadStoreOptimizer, DEBUG_TYPE,
                    "SI Load / Store Optimizer", false, false)

char SILoadStoreOptimizer::ID = 0;

char &llvm::SILoadStoreOptimizerID = SILoadStoreOptimizer::ID;

FunctionPass *llvm::createSILoadStoreOptimizerPass(TargetMachine &TM) {
  return new SILoadStoreOptimizer(TM);
}

bool SILoadStoreOptimizer::offsetsCanBeCombined(unsigned Offset0,
                                                unsigned Offset1,
                                                unsigned EltSize) {
  // XXX - Would the same offset be OK? Is there any reason this would happen or
  // be useful?
  return (Offset0 != Offset1) &&
    isUInt<8>(Offset0 / EltSize) &&
    isUInt<8>(Offset1 / EltSize);
}

MachineBasicBlock::iterator
SILoadStoreOptimizer::findMatchingDSInst(MachineBasicBlock::iterator I,
                                         unsigned EltSize){
  MachineBasicBlock::iterator E = I->getParent()->end();
  MachineBasicBlock::iterator MBBI = I;
  ++MBBI;

  if (MBBI->getOpcode() != I->getOpcode())
    return E;

  // Don't merge volatiles.
  if (MBBI->hasOrderedMemoryRef())
    return E;

  int AddrIdx = AMDGPU::getNamedOperandIdx(I->getOpcode(), AMDGPU::OpName::addr);
  const MachineOperand &AddrReg0 = I->getOperand(AddrIdx);
  const MachineOperand &AddrReg1 = MBBI->getOperand(AddrIdx);

  // Check same base pointer. Be careful of subregisters, which can occur with
  // vectors of pointers.
  if (AddrReg0.getReg() == AddrReg1.getReg() &&
      AddrReg0.getSubReg() == AddrReg1.getSubReg()) {
    int OffsetIdx = AMDGPU::getNamedOperandIdx(I->getOpcode(),
                                               AMDGPU::OpName::offset);
    unsigned Offset0 = I->getOperand(OffsetIdx).getImm();
    unsigned Offset1 = MBBI->getOperand(OffsetIdx).getImm();

    // Check both offsets fit in the reduced range.
    if (offsetsCanBeCombined(Offset0, Offset1, EltSize))
      return MBBI;
  }

  return E;
}

void SILoadStoreOptimizer::updateRegDefsUses(unsigned SrcReg,
                                             unsigned DstReg,
                                             unsigned SubIdx) {
  for (MachineRegisterInfo::reg_iterator I = MRI->reg_begin(SrcReg),
         E = MRI->reg_end(); I != E; ) {
    MachineOperand &O = *I;
    ++I;
    O.substVirtReg(DstReg, SubIdx, *TRI);
  }
}

MachineBasicBlock::iterator  SILoadStoreOptimizer::mergeRead2Pair(
  MachineBasicBlock::iterator I,
  MachineBasicBlock::iterator Paired,
  unsigned EltSize,
  const MCInstrDesc &Read2InstDesc) {
  MachineBasicBlock *MBB = I->getParent();

  // Be careful, since the addresses could be subregisters themselves in weird
  // cases, like vectors of pointers.
  const MachineOperand *AddrReg = TII->getNamedOperand(*I, AMDGPU::OpName::addr);

  unsigned DestReg0 = TII->getNamedOperand(*I, AMDGPU::OpName::vdst)->getReg();
  unsigned DestReg1
    = TII->getNamedOperand(*Paired, AMDGPU::OpName::vdst)->getReg();

  unsigned Offset0 = TII->getNamedOperand(*I, AMDGPU::OpName::offset)->getImm();
  unsigned Offset1
    = TII->getNamedOperand(*Paired, AMDGPU::OpName::offset)->getImm();

  const TargetRegisterClass *SuperRC
    = (EltSize == 4) ? &AMDGPU::VReg_64RegClass : &AMDGPU::VReg_128RegClass;
  unsigned DestReg = MRI->createVirtualRegister(SuperRC);

  DebugLoc DL = I->getDebugLoc();
  MachineInstrBuilder Read2
    = BuildMI(*MBB, I, DL, Read2InstDesc, DestReg)
    .addImm(0) // gds
    .addOperand(*AddrReg) // addr
    .addImm(Offset0 / EltSize) // offset0
    .addImm(Offset1 / EltSize) // offset1
    .addMemOperand(*I->memoperands_begin())
    .addMemOperand(*Paired->memoperands_begin());

  LIS->InsertMachineInstrInMaps(Read2);

  unsigned SubRegIdx0 = (EltSize == 4) ? AMDGPU::sub0 : AMDGPU::sub0_sub1;
  unsigned SubRegIdx1 = (EltSize == 4) ? AMDGPU::sub1 : AMDGPU::sub2_sub3;
  updateRegDefsUses(DestReg0, DestReg, SubRegIdx0);
  updateRegDefsUses(DestReg1, DestReg, SubRegIdx1);

  LIS->RemoveMachineInstrFromMaps(I);
  LIS->RemoveMachineInstrFromMaps(Paired);
  I->eraseFromParent();
  Paired->eraseFromParent();

  LiveInterval &AddrRegLI = LIS->getInterval(AddrReg->getReg());
  LIS->shrinkToUses(&AddrRegLI);

  LIS->getInterval(DestReg); // Create new LI

  DEBUG(dbgs() << "Inserted read2: " << *Read2 << '\n');
  return Read2;
}

MachineBasicBlock::iterator SILoadStoreOptimizer::mergeWrite2Pair(
  MachineBasicBlock::iterator I,
  MachineBasicBlock::iterator Paired,
  unsigned EltSize,
  const MCInstrDesc &Write2InstDesc) {
  MachineBasicBlock *MBB = I->getParent();

  // Be sure to use .addOperand(), and not .addReg() with these. We want to be
  // sure we preserve the subregister index and any register flags set on them.
  const MachineOperand *Addr = TII->getNamedOperand(*I, AMDGPU::OpName::addr);
  const MachineOperand *Data0 = TII->getNamedOperand(*I, AMDGPU::OpName::data0);
  const MachineOperand *Data1
    = TII->getNamedOperand(*Paired, AMDGPU::OpName::data0);

  unsigned Offset0 = TII->getNamedOperand(*I, AMDGPU::OpName::offset)->getImm();
  unsigned Offset1
    = TII->getNamedOperand(*Paired, AMDGPU::OpName::offset)->getImm();

  DebugLoc DL = I->getDebugLoc();
  MachineInstrBuilder Write2
    = BuildMI(*MBB, I, DL, Write2InstDesc)
    .addImm(0) // gds
    .addOperand(*Addr) // addr
    .addOperand(*Data0) // data0
    .addOperand(*Data1) // data1
    .addImm(Offset0 / EltSize) // offset0
    .addImm(Offset1 / EltSize) // offset1
    .addMemOperand(*I->memoperands_begin())
    .addMemOperand(*Paired->memoperands_begin());

  // XXX - How do we express subregisters here?
  unsigned OrigRegs[] = { Data0->getReg(), Data1->getReg(), Addr->getReg() };

  LIS->RemoveMachineInstrFromMaps(I);
  LIS->RemoveMachineInstrFromMaps(Paired);
  I->eraseFromParent();
  Paired->eraseFromParent();

  LIS->repairIntervalsInRange(MBB, Write2, Write2, OrigRegs);

  DEBUG(dbgs() << "Inserted write2 inst: " << *Write2 << '\n');
  return Write2;
}

// Scan through looking for adjacent LDS operations with constant offsets from
// the same base register. We rely on the scheduler to do the hard work of
// clustering nearby loads, and assume these are all adjacent.
bool SILoadStoreOptimizer::optimizeBlock(MachineBasicBlock &MBB) {
  const MCInstrDesc &Read2B32Desc = TII->get(AMDGPU::DS_READ2_B32);
  const MCInstrDesc &Read2B64Desc = TII->get(AMDGPU::DS_READ2_B64);
  const MCInstrDesc &Write2B32Desc = TII->get(AMDGPU::DS_WRITE2_B32);
  const MCInstrDesc &Write2B64Desc = TII->get(AMDGPU::DS_WRITE2_B64);

  bool Modified = false;

  for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E;) {
    MachineInstr &MI = *I;

    // Don't combine if volatile.
    if (MI.hasOrderedMemoryRef()) {
      ++I;
      continue;
    }

    unsigned Opc = MI.getOpcode();
    if (Opc == AMDGPU::DS_READ_B32 || Opc == AMDGPU::DS_READ_B64) {
      unsigned Size = (Opc == AMDGPU::DS_READ_B64) ? 8 : 4;
      MachineBasicBlock::iterator Match = findMatchingDSInst(I, Size);
      if (Match != E) {
        Modified = true;

        const MCInstrDesc &Read2Desc
          = (Opc == AMDGPU::DS_READ_B64) ? Read2B64Desc : Read2B32Desc;
        I = mergeRead2Pair(I, Match, Size, Read2Desc);
      } else {
        ++I;
      }

      continue;
    } else if (Opc == AMDGPU::DS_WRITE_B32 || Opc == AMDGPU::DS_WRITE_B64) {
      unsigned Size = (Opc == AMDGPU::DS_WRITE_B64) ? 8 : 4;
      MachineBasicBlock::iterator Match = findMatchingDSInst(I, Size);
      if (Match != E) {
        Modified = true;

        const MCInstrDesc &Write2Desc
          = (Opc == AMDGPU::DS_WRITE_B64) ? Write2B64Desc : Write2B32Desc;

        I = mergeWrite2Pair(I, Match, Size, Write2Desc);
      } else {
        ++I;
      }

      continue;
    }

    ++I;
  }

  return Modified;
}

bool SILoadStoreOptimizer::runOnMachineFunction(MachineFunction &MF) {
  const TargetSubtargetInfo *STM = MF.getTarget().getSubtargetImpl();
  TRI = static_cast<const SIRegisterInfo*>(STM->getRegisterInfo());
  TII = static_cast<const SIInstrInfo*>(STM->getInstrInfo());
  MRI = &MF.getRegInfo();

  LIS = &getAnalysis<LiveIntervals>();

  DEBUG(dbgs() << "Running SILoadStoreOptimizer\n");

  assert(!MRI->isSSA());

  bool Modified = false;

  for (MachineBasicBlock &MBB : MF)
    Modified |= optimizeBlock(MBB);

  return Modified;
}
