//===-- SelectionDAGISel.cpp - Implement the SelectionDAGISel class -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This implements the SelectionDAGISel class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "isel"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include "llvm/CallingConv.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/InlineAsm.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/CodeGen/MachineDebugInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/SchedulerRegistry.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SSARegMap.h"
#include "llvm/Target/MRegisterInfo.h"
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetFrameInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>
using namespace llvm;

#ifndef NDEBUG
static cl::opt<bool>
ViewISelDAGs("view-isel-dags", cl::Hidden,
          cl::desc("Pop up a window to show isel dags as they are selected"));
static cl::opt<bool>
ViewSchedDAGs("view-sched-dags", cl::Hidden,
          cl::desc("Pop up a window to show sched dags as they are processed"));
#else
static const bool ViewISelDAGs = 0, ViewSchedDAGs = 0;
#endif


//===---------------------------------------------------------------------===//
///
/// RegisterScheduler class - Track the registration of instruction schedulers.
///
//===---------------------------------------------------------------------===//
MachinePassRegistry RegisterScheduler::Registry;

//===---------------------------------------------------------------------===//
///
/// ISHeuristic command line option for instruction schedulers.
///
//===---------------------------------------------------------------------===//
namespace {
  cl::opt<RegisterScheduler::FunctionPassCtor, false,
          RegisterPassParser<RegisterScheduler> >
  ISHeuristic("sched",
              cl::init(&createDefaultScheduler),
              cl::desc("Instruction schedulers available:"));

  static RegisterScheduler
  defaultListDAGScheduler("default", "  Best scheduler for the target",
                          createDefaultScheduler);
} // namespace

namespace {
  /// RegsForValue - This struct represents the physical registers that a
  /// particular value is assigned and the type information about the value.
  /// This is needed because values can be promoted into larger registers and
  /// expanded into multiple smaller registers than the value.
  struct VISIBILITY_HIDDEN RegsForValue {
    /// Regs - This list hold the register (for legal and promoted values)
    /// or register set (for expanded values) that the value should be assigned
    /// to.
    std::vector<unsigned> Regs;
    
    /// RegVT - The value type of each register.
    ///
    MVT::ValueType RegVT;
    
    /// ValueVT - The value type of the LLVM value, which may be promoted from
    /// RegVT or made from merging the two expanded parts.
    MVT::ValueType ValueVT;
    
    RegsForValue() : RegVT(MVT::Other), ValueVT(MVT::Other) {}
    
    RegsForValue(unsigned Reg, MVT::ValueType regvt, MVT::ValueType valuevt)
      : RegVT(regvt), ValueVT(valuevt) {
        Regs.push_back(Reg);
    }
    RegsForValue(const std::vector<unsigned> &regs, 
                 MVT::ValueType regvt, MVT::ValueType valuevt)
      : Regs(regs), RegVT(regvt), ValueVT(valuevt) {
    }
    
    /// getCopyFromRegs - Emit a series of CopyFromReg nodes that copies from
    /// this value and returns the result as a ValueVT value.  This uses 
    /// Chain/Flag as the input and updates them for the output Chain/Flag.
    SDOperand getCopyFromRegs(SelectionDAG &DAG,
                              SDOperand &Chain, SDOperand &Flag) const;

    /// getCopyToRegs - Emit a series of CopyToReg nodes that copies the
    /// specified value into the registers specified by this object.  This uses 
    /// Chain/Flag as the input and updates them for the output Chain/Flag.
    void getCopyToRegs(SDOperand Val, SelectionDAG &DAG,
                       SDOperand &Chain, SDOperand &Flag,
                       MVT::ValueType PtrVT) const;
    
    /// AddInlineAsmOperands - Add this value to the specified inlineasm node
    /// operand list.  This adds the code marker and includes the number of 
    /// values added into it.
    void AddInlineAsmOperands(unsigned Code, SelectionDAG &DAG,
                              std::vector<SDOperand> &Ops) const;
  };
}

namespace llvm {
  //===--------------------------------------------------------------------===//
  /// createDefaultScheduler - This creates an instruction scheduler appropriate
  /// for the target.
  ScheduleDAG* createDefaultScheduler(SelectionDAGISel *IS,
                                      SelectionDAG *DAG,
                                      MachineBasicBlock *BB) {
    TargetLowering &TLI = IS->getTargetLowering();
    
    if (TLI.getSchedulingPreference() == TargetLowering::SchedulingForLatency) {
      return createTDListDAGScheduler(IS, DAG, BB);
    } else {
      assert(TLI.getSchedulingPreference() ==
           TargetLowering::SchedulingForRegPressure && "Unknown sched type!");
      return createBURRListDAGScheduler(IS, DAG, BB);
    }
  }


  //===--------------------------------------------------------------------===//
  /// FunctionLoweringInfo - This contains information that is global to a
  /// function that is used when lowering a region of the function.
  class FunctionLoweringInfo {
  public:
    TargetLowering &TLI;
    Function &Fn;
    MachineFunction &MF;
    SSARegMap *RegMap;

    FunctionLoweringInfo(TargetLowering &TLI, Function &Fn,MachineFunction &MF);

    /// MBBMap - A mapping from LLVM basic blocks to their machine code entry.
    std::map<const BasicBlock*, MachineBasicBlock *> MBBMap;

    /// ValueMap - Since we emit code for the function a basic block at a time,
    /// we must remember which virtual registers hold the values for
    /// cross-basic-block values.
    std::map<const Value*, unsigned> ValueMap;

    /// StaticAllocaMap - Keep track of frame indices for fixed sized allocas in
    /// the entry block.  This allows the allocas to be efficiently referenced
    /// anywhere in the function.
    std::map<const AllocaInst*, int> StaticAllocaMap;

    unsigned MakeReg(MVT::ValueType VT) {
      return RegMap->createVirtualRegister(TLI.getRegClassFor(VT));
    }
    
    /// isExportedInst - Return true if the specified value is an instruction
    /// exported from its block.
    bool isExportedInst(const Value *V) {
      return ValueMap.count(V);
    }

    unsigned CreateRegForValue(const Value *V);
    
    unsigned InitializeRegForValue(const Value *V) {
      unsigned &R = ValueMap[V];
      assert(R == 0 && "Already initialized this value register!");
      return R = CreateRegForValue(V);
    }
  };
}

/// isUsedOutsideOfDefiningBlock - Return true if this instruction is used by
/// PHI nodes or outside of the basic block that defines it, or used by a 
/// switch instruction, which may expand to multiple basic blocks.
static bool isUsedOutsideOfDefiningBlock(Instruction *I) {
  if (isa<PHINode>(I)) return true;
  BasicBlock *BB = I->getParent();
  for (Value::use_iterator UI = I->use_begin(), E = I->use_end(); UI != E; ++UI)
    if (cast<Instruction>(*UI)->getParent() != BB || isa<PHINode>(*UI) ||
        // FIXME: Remove switchinst special case.
        isa<SwitchInst>(*UI))
      return true;
  return false;
}

/// isOnlyUsedInEntryBlock - If the specified argument is only used in the
/// entry block, return true.  This includes arguments used by switches, since
/// the switch may expand into multiple basic blocks.
static bool isOnlyUsedInEntryBlock(Argument *A) {
  BasicBlock *Entry = A->getParent()->begin();
  for (Value::use_iterator UI = A->use_begin(), E = A->use_end(); UI != E; ++UI)
    if (cast<Instruction>(*UI)->getParent() != Entry || isa<SwitchInst>(*UI))
      return false;  // Use not in entry block.
  return true;
}

FunctionLoweringInfo::FunctionLoweringInfo(TargetLowering &tli,
                                           Function &fn, MachineFunction &mf)
    : TLI(tli), Fn(fn), MF(mf), RegMap(MF.getSSARegMap()) {

  // Create a vreg for each argument register that is not dead and is used
  // outside of the entry block for the function.
  for (Function::arg_iterator AI = Fn.arg_begin(), E = Fn.arg_end();
       AI != E; ++AI)
    if (!isOnlyUsedInEntryBlock(AI))
      InitializeRegForValue(AI);

  // Initialize the mapping of values to registers.  This is only set up for
  // instruction values that are used outside of the block that defines
  // them.
  Function::iterator BB = Fn.begin(), EB = Fn.end();
  for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
    if (AllocaInst *AI = dyn_cast<AllocaInst>(I))
      if (ConstantInt *CUI = dyn_cast<ConstantInt>(AI->getArraySize())) {
        const Type *Ty = AI->getAllocatedType();
        uint64_t TySize = TLI.getTargetData()->getTypeSize(Ty);
        unsigned Align = 
          std::max((unsigned)TLI.getTargetData()->getTypeAlignment(Ty),
                   AI->getAlignment());

        // If the alignment of the value is smaller than the size of the 
        // value, and if the size of the value is particularly small 
        // (<= 8 bytes), round up to the size of the value for potentially 
        // better performance.
        //
        // FIXME: This could be made better with a preferred alignment hook in
        // TargetData.  It serves primarily to 8-byte align doubles for X86.
        if (Align < TySize && TySize <= 8) Align = TySize;
        TySize *= CUI->getZExtValue();   // Get total allocated size.
        if (TySize == 0) TySize = 1; // Don't create zero-sized stack objects.
        StaticAllocaMap[AI] =
          MF.getFrameInfo()->CreateStackObject((unsigned)TySize, Align);
      }

  for (; BB != EB; ++BB)
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
      if (!I->use_empty() && isUsedOutsideOfDefiningBlock(I))
        if (!isa<AllocaInst>(I) ||
            !StaticAllocaMap.count(cast<AllocaInst>(I)))
          InitializeRegForValue(I);

  // Create an initial MachineBasicBlock for each LLVM BasicBlock in F.  This
  // also creates the initial PHI MachineInstrs, though none of the input
  // operands are populated.
  for (BB = Fn.begin(), EB = Fn.end(); BB != EB; ++BB) {
    MachineBasicBlock *MBB = new MachineBasicBlock(BB);
    MBBMap[BB] = MBB;
    MF.getBasicBlockList().push_back(MBB);

    // Create Machine PHI nodes for LLVM PHI nodes, lowering them as
    // appropriate.
    PHINode *PN;
    for (BasicBlock::iterator I = BB->begin();(PN = dyn_cast<PHINode>(I)); ++I){
      if (PN->use_empty()) continue;
      
      MVT::ValueType VT = TLI.getValueType(PN->getType());
      unsigned NumElements;
      if (VT != MVT::Vector)
        NumElements = TLI.getNumElements(VT);
      else {
        MVT::ValueType VT1,VT2;
        NumElements = 
          TLI.getPackedTypeBreakdown(cast<PackedType>(PN->getType()),
                                     VT1, VT2);
      }
      unsigned PHIReg = ValueMap[PN];
      assert(PHIReg && "PHI node does not have an assigned virtual register!");
      const TargetInstrInfo *TII = TLI.getTargetMachine().getInstrInfo();
      for (unsigned i = 0; i != NumElements; ++i)
        BuildMI(MBB, TII->get(TargetInstrInfo::PHI), PHIReg+i);
    }
  }
}

/// CreateRegForValue - Allocate the appropriate number of virtual registers of
/// the correctly promoted or expanded types.  Assign these registers
/// consecutive vreg numbers and return the first assigned number.
unsigned FunctionLoweringInfo::CreateRegForValue(const Value *V) {
  MVT::ValueType VT = TLI.getValueType(V->getType());
  
  // The number of multiples of registers that we need, to, e.g., split up
  // a <2 x int64> -> 4 x i32 registers.
  unsigned NumVectorRegs = 1;
  
  // If this is a packed type, figure out what type it will decompose into
  // and how many of the elements it will use.
  if (VT == MVT::Vector) {
    const PackedType *PTy = cast<PackedType>(V->getType());
    unsigned NumElts = PTy->getNumElements();
    MVT::ValueType EltTy = TLI.getValueType(PTy->getElementType());
    
    // Divide the input until we get to a supported size.  This will always
    // end with a scalar if the target doesn't support vectors.
    while (NumElts > 1 && !TLI.isTypeLegal(getVectorType(EltTy, NumElts))) {
      NumElts >>= 1;
      NumVectorRegs <<= 1;
    }
    if (NumElts == 1)
      VT = EltTy;
    else
      VT = getVectorType(EltTy, NumElts);
  }
  
  // The common case is that we will only create one register for this
  // value.  If we have that case, create and return the virtual register.
  unsigned NV = TLI.getNumElements(VT);
  if (NV == 1) {
    // If we are promoting this value, pick the next largest supported type.
    MVT::ValueType PromotedType = TLI.getTypeToTransformTo(VT);
    unsigned Reg = MakeReg(PromotedType);
    // If this is a vector of supported or promoted types (e.g. 4 x i16),
    // create all of the registers.
    for (unsigned i = 1; i != NumVectorRegs; ++i)
      MakeReg(PromotedType);
    return Reg;
  }
  
  // If this value is represented with multiple target registers, make sure
  // to create enough consecutive registers of the right (smaller) type.
  VT = TLI.getTypeToExpandTo(VT);
  unsigned R = MakeReg(VT);
  for (unsigned i = 1; i != NV*NumVectorRegs; ++i)
    MakeReg(VT);
  return R;
}

//===----------------------------------------------------------------------===//
/// SelectionDAGLowering - This is the common target-independent lowering
/// implementation that is parameterized by a TargetLowering object.
/// Also, targets can overload any lowering method.
///
namespace llvm {
class SelectionDAGLowering {
  MachineBasicBlock *CurMBB;

  std::map<const Value*, SDOperand> NodeMap;

  /// PendingLoads - Loads are not emitted to the program immediately.  We bunch
  /// them up and then emit token factor nodes when possible.  This allows us to
  /// get simple disambiguation between loads without worrying about alias
  /// analysis.
  std::vector<SDOperand> PendingLoads;

  /// Case - A pair of values to record the Value for a switch case, and the
  /// case's target basic block.  
  typedef std::pair<Constant*, MachineBasicBlock*> Case;
  typedef std::vector<Case>::iterator              CaseItr;
  typedef std::pair<CaseItr, CaseItr>              CaseRange;

  /// CaseRec - A struct with ctor used in lowering switches to a binary tree
  /// of conditional branches.
  struct CaseRec {
    CaseRec(MachineBasicBlock *bb, Constant *lt, Constant *ge, CaseRange r) :
    CaseBB(bb), LT(lt), GE(ge), Range(r) {}

    /// CaseBB - The MBB in which to emit the compare and branch
    MachineBasicBlock *CaseBB;
    /// LT, GE - If nonzero, we know the current case value must be less-than or
    /// greater-than-or-equal-to these Constants.
    Constant *LT;
    Constant *GE;
    /// Range - A pair of iterators representing the range of case values to be
    /// processed at this point in the binary search tree.
    CaseRange Range;
  };
  
  /// The comparison function for sorting Case values.
  struct CaseCmp {
    bool operator () (const Case& C1, const Case& C2) {
      if (const ConstantInt* I1 = dyn_cast<const ConstantInt>(C1.first))
        if (I1->getType()->isUnsigned())
          return I1->getZExtValue() <
            cast<const ConstantInt>(C2.first)->getZExtValue();
      
      return cast<const ConstantInt>(C1.first)->getSExtValue() <
         cast<const ConstantInt>(C2.first)->getSExtValue();
    }
  };
  
public:
  // TLI - This is information that describes the available target features we
  // need for lowering.  This indicates when operations are unavailable,
  // implemented with a libcall, etc.
  TargetLowering &TLI;
  SelectionDAG &DAG;
  const TargetData *TD;

  /// SwitchCases - Vector of CaseBlock structures used to communicate
  /// SwitchInst code generation information.
  std::vector<SelectionDAGISel::CaseBlock> SwitchCases;
  SelectionDAGISel::JumpTable JT;
  
  /// FuncInfo - Information about the function as a whole.
  ///
  FunctionLoweringInfo &FuncInfo;

  SelectionDAGLowering(SelectionDAG &dag, TargetLowering &tli,
                       FunctionLoweringInfo &funcinfo)
    : TLI(tli), DAG(dag), TD(DAG.getTarget().getTargetData()),
      JT(0,0,0,0), FuncInfo(funcinfo) {
  }

  /// getRoot - Return the current virtual root of the Selection DAG.
  ///
  SDOperand getRoot() {
    if (PendingLoads.empty())
      return DAG.getRoot();

    if (PendingLoads.size() == 1) {
      SDOperand Root = PendingLoads[0];
      DAG.setRoot(Root);
      PendingLoads.clear();
      return Root;
    }

    // Otherwise, we have to make a token factor node.
    SDOperand Root = DAG.getNode(ISD::TokenFactor, MVT::Other,
                                 &PendingLoads[0], PendingLoads.size());
    PendingLoads.clear();
    DAG.setRoot(Root);
    return Root;
  }

  SDOperand CopyValueToVirtualRegister(Value *V, unsigned Reg);

  void visit(Instruction &I) { visit(I.getOpcode(), I); }

  void visit(unsigned Opcode, User &I) {
    // Note: this doesn't use InstVisitor, because it has to work with
    // ConstantExpr's in addition to instructions.
    switch (Opcode) {
    default: assert(0 && "Unknown instruction type encountered!");
             abort();
      // Build the switch statement using the Instruction.def file.
#define HANDLE_INST(NUM, OPCODE, CLASS) \
    case Instruction::OPCODE:return visit##OPCODE((CLASS&)I);
#include "llvm/Instruction.def"
    }
  }

  void setCurrentBasicBlock(MachineBasicBlock *MBB) { CurMBB = MBB; }

  SDOperand getLoadFrom(const Type *Ty, SDOperand Ptr,
                        const Value *SV, SDOperand Root,
                        bool isVolatile);

  SDOperand getIntPtrConstant(uint64_t Val) {
    return DAG.getConstant(Val, TLI.getPointerTy());
  }

  SDOperand getValue(const Value *V);

  const SDOperand &setValue(const Value *V, SDOperand NewN) {
    SDOperand &N = NodeMap[V];
    assert(N.Val == 0 && "Already set a value for this node!");
    return N = NewN;
  }
  
  RegsForValue GetRegistersForValue(const std::string &ConstrCode,
                                    MVT::ValueType VT,
                                    bool OutReg, bool InReg,
                                    std::set<unsigned> &OutputRegs, 
                                    std::set<unsigned> &InputRegs);

  void FindMergedConditions(Value *Cond, MachineBasicBlock *TBB,
                            MachineBasicBlock *FBB, MachineBasicBlock *CurBB,
                            unsigned Opc);
  bool isExportableFromCurrentBlock(Value *V, const BasicBlock *FromBB);
  void ExportFromCurrentBlock(Value *V);
    
  // Terminator instructions.
  void visitRet(ReturnInst &I);
  void visitBr(BranchInst &I);
  void visitSwitch(SwitchInst &I);
  void visitUnreachable(UnreachableInst &I) { /* noop */ }

  // Helper for visitSwitch
  void visitSwitchCase(SelectionDAGISel::CaseBlock &CB);
  void visitJumpTable(SelectionDAGISel::JumpTable &JT);
  
  // These all get lowered before this pass.
  void visitInvoke(InvokeInst &I) { assert(0 && "TODO"); }
  void visitUnwind(UnwindInst &I) { assert(0 && "TODO"); }

  void visitIntBinary(User &I, unsigned IntOp, unsigned VecOp);
  void visitFPBinary(User &I, unsigned FPOp, unsigned VecOp);
  void visitShift(User &I, unsigned Opcode);
  void visitAdd(User &I) { 
    if (I.getType()->isFloatingPoint())
      visitFPBinary(I, ISD::FADD, ISD::VADD); 
    else
      visitIntBinary(I, ISD::ADD, ISD::VADD); 
  }
  void visitSub(User &I);
  void visitMul(User &I) {
    if (I.getType()->isFloatingPoint()) 
      visitFPBinary(I, ISD::FMUL, ISD::VMUL); 
    else
      visitIntBinary(I, ISD::MUL, ISD::VMUL); 
  }
  void visitURem(User &I) { visitIntBinary(I, ISD::UREM, 0); }
  void visitSRem(User &I) { visitIntBinary(I, ISD::SREM, 0); }
  void visitFRem(User &I) { visitFPBinary (I, ISD::FREM, 0); }
  void visitUDiv(User &I) { visitIntBinary(I, ISD::UDIV, ISD::VUDIV); }
  void visitSDiv(User &I) { visitIntBinary(I, ISD::SDIV, ISD::VSDIV); }
  void visitFDiv(User &I) { visitFPBinary (I, ISD::FDIV, ISD::VSDIV); }
  void visitAnd(User &I) { visitIntBinary(I, ISD::AND, ISD::VAND); }
  void visitOr (User &I) { visitIntBinary(I, ISD::OR,  ISD::VOR); }
  void visitXor(User &I) { visitIntBinary(I, ISD::XOR, ISD::VXOR); }
  void visitShl(User &I) { visitShift(I, ISD::SHL); }
  void visitLShr(User &I) { visitShift(I, ISD::SRL); }
  void visitAShr(User &I) { visitShift(I, ISD::SRA); }
  void visitICmp(User &I);
  void visitFCmp(User &I);
  void visitSetCC(User &I, ISD::CondCode SignedOpc, ISD::CondCode UnsignedOpc,
                  ISD::CondCode FPOpc);
  void visitSetEQ(User &I) { visitSetCC(I, ISD::SETEQ, ISD::SETEQ, 
                                        ISD::SETOEQ); }
  void visitSetNE(User &I) { visitSetCC(I, ISD::SETNE, ISD::SETNE,
                                        ISD::SETUNE); }
  void visitSetLE(User &I) { visitSetCC(I, ISD::SETLE, ISD::SETULE,
                                        ISD::SETOLE); }
  void visitSetGE(User &I) { visitSetCC(I, ISD::SETGE, ISD::SETUGE,
                                        ISD::SETOGE); }
  void visitSetLT(User &I) { visitSetCC(I, ISD::SETLT, ISD::SETULT,
                                        ISD::SETOLT); }
  void visitSetGT(User &I) { visitSetCC(I, ISD::SETGT, ISD::SETUGT,
                                        ISD::SETOGT); }
  // Visit the conversion instructions
  void visitTrunc(User &I);
  void visitZExt(User &I);
  void visitSExt(User &I);
  void visitFPTrunc(User &I);
  void visitFPExt(User &I);
  void visitFPToUI(User &I);
  void visitFPToSI(User &I);
  void visitUIToFP(User &I);
  void visitSIToFP(User &I);
  void visitPtrToInt(User &I);
  void visitIntToPtr(User &I);
  void visitBitCast(User &I);

  void visitExtractElement(User &I);
  void visitInsertElement(User &I);
  void visitShuffleVector(User &I);

  void visitGetElementPtr(User &I);
  void visitSelect(User &I);

  void visitMalloc(MallocInst &I);
  void visitFree(FreeInst &I);
  void visitAlloca(AllocaInst &I);
  void visitLoad(LoadInst &I);
  void visitStore(StoreInst &I);
  void visitPHI(PHINode &I) { } // PHI nodes are handled specially.
  void visitCall(CallInst &I);
  void visitInlineAsm(CallInst &I);
  const char *visitIntrinsicCall(CallInst &I, unsigned Intrinsic);
  void visitTargetIntrinsic(CallInst &I, unsigned Intrinsic);

  void visitVAStart(CallInst &I);
  void visitVAArg(VAArgInst &I);
  void visitVAEnd(CallInst &I);
  void visitVACopy(CallInst &I);
  void visitFrameReturnAddress(CallInst &I, bool isFrameAddress);

  void visitMemIntrinsic(CallInst &I, unsigned Op);

  void visitUserOp1(Instruction &I) {
    assert(0 && "UserOp1 should not exist at instruction selection time!");
    abort();
  }
  void visitUserOp2(Instruction &I) {
    assert(0 && "UserOp2 should not exist at instruction selection time!");
    abort();
  }
};
} // end namespace llvm

SDOperand SelectionDAGLowering::getValue(const Value *V) {
  SDOperand &N = NodeMap[V];
  if (N.Val) return N;
  
  const Type *VTy = V->getType();
  MVT::ValueType VT = TLI.getValueType(VTy);
  if (Constant *C = const_cast<Constant*>(dyn_cast<Constant>(V))) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
      visit(CE->getOpcode(), *CE);
      assert(N.Val && "visit didn't populate the ValueMap!");
      return N;
    } else if (GlobalValue *GV = dyn_cast<GlobalValue>(C)) {
      return N = DAG.getGlobalAddress(GV, VT);
    } else if (isa<ConstantPointerNull>(C)) {
      return N = DAG.getConstant(0, TLI.getPointerTy());
    } else if (isa<UndefValue>(C)) {
      if (!isa<PackedType>(VTy))
        return N = DAG.getNode(ISD::UNDEF, VT);

      // Create a VBUILD_VECTOR of undef nodes.
      const PackedType *PTy = cast<PackedType>(VTy);
      unsigned NumElements = PTy->getNumElements();
      MVT::ValueType PVT = TLI.getValueType(PTy->getElementType());

      SmallVector<SDOperand, 8> Ops;
      Ops.assign(NumElements, DAG.getNode(ISD::UNDEF, PVT));
      
      // Create a VConstant node with generic Vector type.
      Ops.push_back(DAG.getConstant(NumElements, MVT::i32));
      Ops.push_back(DAG.getValueType(PVT));
      return N = DAG.getNode(ISD::VBUILD_VECTOR, MVT::Vector,
                             &Ops[0], Ops.size());
    } else if (ConstantFP *CFP = dyn_cast<ConstantFP>(C)) {
      return N = DAG.getConstantFP(CFP->getValue(), VT);
    } else if (const PackedType *PTy = dyn_cast<PackedType>(VTy)) {
      unsigned NumElements = PTy->getNumElements();
      MVT::ValueType PVT = TLI.getValueType(PTy->getElementType());
      
      // Now that we know the number and type of the elements, push a
      // Constant or ConstantFP node onto the ops list for each element of
      // the packed constant.
      SmallVector<SDOperand, 8> Ops;
      if (ConstantPacked *CP = dyn_cast<ConstantPacked>(C)) {
        for (unsigned i = 0; i != NumElements; ++i)
          Ops.push_back(getValue(CP->getOperand(i)));
      } else {
        assert(isa<ConstantAggregateZero>(C) && "Unknown packed constant!");
        SDOperand Op;
        if (MVT::isFloatingPoint(PVT))
          Op = DAG.getConstantFP(0, PVT);
        else
          Op = DAG.getConstant(0, PVT);
        Ops.assign(NumElements, Op);
      }
      
      // Create a VBUILD_VECTOR node with generic Vector type.
      Ops.push_back(DAG.getConstant(NumElements, MVT::i32));
      Ops.push_back(DAG.getValueType(PVT));
      return N = DAG.getNode(ISD::VBUILD_VECTOR,MVT::Vector,&Ops[0],Ops.size());
    } else {
      // Canonicalize all constant ints to be unsigned.
      return N = DAG.getConstant(cast<ConstantIntegral>(C)->getZExtValue(),VT);
    }
  }
      
  if (const AllocaInst *AI = dyn_cast<AllocaInst>(V)) {
    std::map<const AllocaInst*, int>::iterator SI =
    FuncInfo.StaticAllocaMap.find(AI);
    if (SI != FuncInfo.StaticAllocaMap.end())
      return DAG.getFrameIndex(SI->second, TLI.getPointerTy());
  }
      
  std::map<const Value*, unsigned>::const_iterator VMI =
      FuncInfo.ValueMap.find(V);
  assert(VMI != FuncInfo.ValueMap.end() && "Value not in map!");
  
  unsigned InReg = VMI->second;
  
  // If this type is not legal, make it so now.
  if (VT != MVT::Vector) {
    if (TLI.getTypeAction(VT) == TargetLowering::Expand) {
      // Source must be expanded.  This input value is actually coming from the
      // register pair VMI->second and VMI->second+1.
      MVT::ValueType DestVT = TLI.getTypeToExpandTo(VT);
      unsigned NumVals = TLI.getNumElements(VT);
      N = DAG.getCopyFromReg(DAG.getEntryNode(), InReg, DestVT);
      if (NumVals == 1)
        N = DAG.getNode(ISD::BIT_CONVERT, VT, N);
      else {
        assert(NumVals == 2 && "1 to 4 (and more) expansion not implemented!");
        N = DAG.getNode(ISD::BUILD_PAIR, VT, N,
                       DAG.getCopyFromReg(DAG.getEntryNode(), InReg+1, DestVT));
      }
    } else {
      MVT::ValueType DestVT = TLI.getTypeToTransformTo(VT);
      N = DAG.getCopyFromReg(DAG.getEntryNode(), InReg, DestVT);
      if (TLI.getTypeAction(VT) == TargetLowering::Promote) // Promotion case
        N = MVT::isFloatingPoint(VT)
          ? DAG.getNode(ISD::FP_ROUND, VT, N)
          : DAG.getNode(ISD::TRUNCATE, VT, N);
    }
  } else {
    // Otherwise, if this is a vector, make it available as a generic vector
    // here.
    MVT::ValueType PTyElementVT, PTyLegalElementVT;
    const PackedType *PTy = cast<PackedType>(VTy);
    unsigned NE = TLI.getPackedTypeBreakdown(PTy, PTyElementVT,
                                             PTyLegalElementVT);

    // Build a VBUILD_VECTOR with the input registers.
    SmallVector<SDOperand, 8> Ops;
    if (PTyElementVT == PTyLegalElementVT) {
      // If the value types are legal, just VBUILD the CopyFromReg nodes.
      for (unsigned i = 0; i != NE; ++i)
        Ops.push_back(DAG.getCopyFromReg(DAG.getEntryNode(), InReg++, 
                                         PTyElementVT));
    } else if (PTyElementVT < PTyLegalElementVT) {
      // If the register was promoted, use TRUNCATE of FP_ROUND as appropriate.
      for (unsigned i = 0; i != NE; ++i) {
        SDOperand Op = DAG.getCopyFromReg(DAG.getEntryNode(), InReg++, 
                                          PTyElementVT);
        if (MVT::isFloatingPoint(PTyElementVT))
          Op = DAG.getNode(ISD::FP_ROUND, PTyElementVT, Op);
        else
          Op = DAG.getNode(ISD::TRUNCATE, PTyElementVT, Op);
        Ops.push_back(Op);
      }
    } else {
      // If the register was expanded, use BUILD_PAIR.
      assert((NE & 1) == 0 && "Must expand into a multiple of 2 elements!");
      for (unsigned i = 0; i != NE/2; ++i) {
        SDOperand Op0 = DAG.getCopyFromReg(DAG.getEntryNode(), InReg++, 
                                           PTyElementVT);
        SDOperand Op1 = DAG.getCopyFromReg(DAG.getEntryNode(), InReg++, 
                                           PTyElementVT);
        Ops.push_back(DAG.getNode(ISD::BUILD_PAIR, VT, Op0, Op1));
      }
    }
    
    Ops.push_back(DAG.getConstant(NE, MVT::i32));
    Ops.push_back(DAG.getValueType(PTyLegalElementVT));
    N = DAG.getNode(ISD::VBUILD_VECTOR, MVT::Vector, &Ops[0], Ops.size());
    
    // Finally, use a VBIT_CONVERT to make this available as the appropriate
    // vector type.
    N = DAG.getNode(ISD::VBIT_CONVERT, MVT::Vector, N, 
                    DAG.getConstant(PTy->getNumElements(),
                                    MVT::i32),
                    DAG.getValueType(TLI.getValueType(PTy->getElementType())));
  }
  
  return N;
}


void SelectionDAGLowering::visitRet(ReturnInst &I) {
  if (I.getNumOperands() == 0) {
    DAG.setRoot(DAG.getNode(ISD::RET, MVT::Other, getRoot()));
    return;
  }
  SmallVector<SDOperand, 8> NewValues;
  NewValues.push_back(getRoot());
  for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
    SDOperand RetOp = getValue(I.getOperand(i));
    bool isSigned = I.getOperand(i)->getType()->isSigned();
    
    // If this is an integer return value, we need to promote it ourselves to
    // the full width of a register, since LegalizeOp will use ANY_EXTEND rather
    // than sign/zero.
    // FIXME: C calling convention requires the return type to be promoted to
    // at least 32-bit. But this is not necessary for non-C calling conventions.
    if (MVT::isInteger(RetOp.getValueType()) && 
        RetOp.getValueType() < MVT::i64) {
      MVT::ValueType TmpVT;
      if (TLI.getTypeAction(MVT::i32) == TargetLowering::Promote)
        TmpVT = TLI.getTypeToTransformTo(MVT::i32);
      else
        TmpVT = MVT::i32;

      if (isSigned)
        RetOp = DAG.getNode(ISD::SIGN_EXTEND, TmpVT, RetOp);
      else
        RetOp = DAG.getNode(ISD::ZERO_EXTEND, TmpVT, RetOp);
    }
    NewValues.push_back(RetOp);
    NewValues.push_back(DAG.getConstant(isSigned, MVT::i32));
  }
  DAG.setRoot(DAG.getNode(ISD::RET, MVT::Other,
                          &NewValues[0], NewValues.size()));
}

/// ExportFromCurrentBlock - If this condition isn't known to be exported from
/// the current basic block, add it to ValueMap now so that we'll get a
/// CopyTo/FromReg.
void SelectionDAGLowering::ExportFromCurrentBlock(Value *V) {
  // No need to export constants.
  if (!isa<Instruction>(V) && !isa<Argument>(V)) return;
  
  // Already exported?
  if (FuncInfo.isExportedInst(V)) return;

  unsigned Reg = FuncInfo.InitializeRegForValue(V);
  PendingLoads.push_back(CopyValueToVirtualRegister(V, Reg));
}

bool SelectionDAGLowering::isExportableFromCurrentBlock(Value *V,
                                                    const BasicBlock *FromBB) {
  // The operands of the setcc have to be in this block.  We don't know
  // how to export them from some other block.
  if (Instruction *VI = dyn_cast<Instruction>(V)) {
    // Can export from current BB.
    if (VI->getParent() == FromBB)
      return true;
    
    // Is already exported, noop.
    return FuncInfo.isExportedInst(V);
  }
  
  // If this is an argument, we can export it if the BB is the entry block or
  // if it is already exported.
  if (isa<Argument>(V)) {
    if (FromBB == &FromBB->getParent()->getEntryBlock())
      return true;

    // Otherwise, can only export this if it is already exported.
    return FuncInfo.isExportedInst(V);
  }
  
  // Otherwise, constants can always be exported.
  return true;
}

static bool InBlock(const Value *V, const BasicBlock *BB) {
  if (const Instruction *I = dyn_cast<Instruction>(V))
    return I->getParent() == BB;
  return true;
}

/// FindMergedConditions - If Cond is an expression like 
void SelectionDAGLowering::FindMergedConditions(Value *Cond,
                                                MachineBasicBlock *TBB,
                                                MachineBasicBlock *FBB,
                                                MachineBasicBlock *CurBB,
                                                unsigned Opc) {
  // If this node is not part of the or/and tree, emit it as a branch.
  BinaryOperator *BOp = dyn_cast<BinaryOperator>(Cond);

  if (!BOp || (unsigned)BOp->getOpcode() != Opc || !BOp->hasOneUse() ||
      BOp->getParent() != CurBB->getBasicBlock() ||
      !InBlock(BOp->getOperand(0), CurBB->getBasicBlock()) ||
      !InBlock(BOp->getOperand(1), CurBB->getBasicBlock())) {
    const BasicBlock *BB = CurBB->getBasicBlock();
    
    if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(Cond))
      if ((II->getIntrinsicID() == Intrinsic::isunordered_f32 ||
           II->getIntrinsicID() == Intrinsic::isunordered_f64) &&
          // The operands of the setcc have to be in this block.  We don't know
          // how to export them from some other block.  If this is the first
          // block of the sequence, no exporting is needed.
          (CurBB == CurMBB ||
           (isExportableFromCurrentBlock(II->getOperand(1), BB) &&
            isExportableFromCurrentBlock(II->getOperand(2), BB)))) {
        SelectionDAGISel::CaseBlock CB(ISD::SETUO, II->getOperand(1),
                                       II->getOperand(2), TBB, FBB, CurBB);
        SwitchCases.push_back(CB);
        return;
      }
        
    
    // If the leaf of the tree is a setcond inst, merge the condition into the
    // caseblock.
    if (BOp && isa<SetCondInst>(BOp) &&
        // The operands of the setcc have to be in this block.  We don't know
        // how to export them from some other block.  If this is the first block
        // of the sequence, no exporting is needed.
        (CurBB == CurMBB ||
         (isExportableFromCurrentBlock(BOp->getOperand(0), BB) &&
          isExportableFromCurrentBlock(BOp->getOperand(1), BB)))) {
      ISD::CondCode SignCond, UnsCond, FPCond, Condition;
      switch (BOp->getOpcode()) {
      default: assert(0 && "Unknown setcc opcode!");
      case Instruction::SetEQ:
        SignCond = ISD::SETEQ;
        UnsCond  = ISD::SETEQ;
        FPCond   = ISD::SETOEQ;
        break;
      case Instruction::SetNE:
        SignCond = ISD::SETNE;
        UnsCond  = ISD::SETNE;
        FPCond   = ISD::SETUNE;
        break;
      case Instruction::SetLE:
        SignCond = ISD::SETLE;
        UnsCond  = ISD::SETULE;
        FPCond   = ISD::SETOLE;
        break;
      case Instruction::SetGE:
        SignCond = ISD::SETGE;
        UnsCond  = ISD::SETUGE;
        FPCond   = ISD::SETOGE;
        break;
      case Instruction::SetLT:
        SignCond = ISD::SETLT;
        UnsCond  = ISD::SETULT;
        FPCond   = ISD::SETOLT;
        break;
      case Instruction::SetGT:
        SignCond = ISD::SETGT;
        UnsCond  = ISD::SETUGT;
        FPCond   = ISD::SETOGT;
        break;
      }
      
      const Type *OpType = BOp->getOperand(0)->getType();
      if (const PackedType *PTy = dyn_cast<PackedType>(OpType))
        OpType = PTy->getElementType();
      
      if (!FiniteOnlyFPMath() && OpType->isFloatingPoint())
        Condition = FPCond;
      else if (OpType->isUnsigned())
        Condition = UnsCond;
      else
        Condition = SignCond;
      
      SelectionDAGISel::CaseBlock CB(Condition, BOp->getOperand(0), 
                                     BOp->getOperand(1), TBB, FBB, CurBB);
      SwitchCases.push_back(CB);
      return;
    }
    
    // Create a CaseBlock record representing this branch.
    SelectionDAGISel::CaseBlock CB(ISD::SETEQ, Cond, ConstantBool::getTrue(),
                                   TBB, FBB, CurBB);
    SwitchCases.push_back(CB);
    return;
  }
  
  
  //  Create TmpBB after CurBB.
  MachineFunction::iterator BBI = CurBB;
  MachineBasicBlock *TmpBB = new MachineBasicBlock(CurBB->getBasicBlock());
  CurBB->getParent()->getBasicBlockList().insert(++BBI, TmpBB);
  
  if (Opc == Instruction::Or) {
    // Codegen X | Y as:
    //   jmp_if_X TBB
    //   jmp TmpBB
    // TmpBB:
    //   jmp_if_Y TBB
    //   jmp FBB
    //
  
    // Emit the LHS condition.
    FindMergedConditions(BOp->getOperand(0), TBB, TmpBB, CurBB, Opc);
  
    // Emit the RHS condition into TmpBB.
    FindMergedConditions(BOp->getOperand(1), TBB, FBB, TmpBB, Opc);
  } else {
    assert(Opc == Instruction::And && "Unknown merge op!");
    // Codegen X & Y as:
    //   jmp_if_X TmpBB
    //   jmp FBB
    // TmpBB:
    //   jmp_if_Y TBB
    //   jmp FBB
    //
    //  This requires creation of TmpBB after CurBB.
    
    // Emit the LHS condition.
    FindMergedConditions(BOp->getOperand(0), TmpBB, FBB, CurBB, Opc);
    
    // Emit the RHS condition into TmpBB.
    FindMergedConditions(BOp->getOperand(1), TBB, FBB, TmpBB, Opc);
  }
}

/// If the set of cases should be emitted as a series of branches, return true.
/// If we should emit this as a bunch of and/or'd together conditions, return
/// false.
static bool 
ShouldEmitAsBranches(const std::vector<SelectionDAGISel::CaseBlock> &Cases) {
  if (Cases.size() != 2) return true;
  
  // If this is two comparisons of the same values or'd or and'd together, they
  // will get folded into a single comparison, so don't emit two blocks.
  if ((Cases[0].CmpLHS == Cases[1].CmpLHS &&
       Cases[0].CmpRHS == Cases[1].CmpRHS) ||
      (Cases[0].CmpRHS == Cases[1].CmpLHS &&
       Cases[0].CmpLHS == Cases[1].CmpRHS)) {
    return false;
  }
  
  return true;
}

void SelectionDAGLowering::visitBr(BranchInst &I) {
  // Update machine-CFG edges.
  MachineBasicBlock *Succ0MBB = FuncInfo.MBBMap[I.getSuccessor(0)];

  // Figure out which block is immediately after the current one.
  MachineBasicBlock *NextBlock = 0;
  MachineFunction::iterator BBI = CurMBB;
  if (++BBI != CurMBB->getParent()->end())
    NextBlock = BBI;

  if (I.isUnconditional()) {
    // If this is not a fall-through branch, emit the branch.
    if (Succ0MBB != NextBlock)
      DAG.setRoot(DAG.getNode(ISD::BR, MVT::Other, getRoot(),
                              DAG.getBasicBlock(Succ0MBB)));

    // Update machine-CFG edges.
    CurMBB->addSuccessor(Succ0MBB);

    return;
  }

  // If this condition is one of the special cases we handle, do special stuff
  // now.
  Value *CondVal = I.getCondition();
  MachineBasicBlock *Succ1MBB = FuncInfo.MBBMap[I.getSuccessor(1)];

  // If this is a series of conditions that are or'd or and'd together, emit
  // this as a sequence of branches instead of setcc's with and/or operations.
  // For example, instead of something like:
  //     cmp A, B
  //     C = seteq 
  //     cmp D, E
  //     F = setle 
  //     or C, F
  //     jnz foo
  // Emit:
  //     cmp A, B
  //     je foo
  //     cmp D, E
  //     jle foo
  //
  if (BinaryOperator *BOp = dyn_cast<BinaryOperator>(CondVal)) {
    if (BOp->hasOneUse() && 
        (BOp->getOpcode() == Instruction::And ||
         BOp->getOpcode() == Instruction::Or)) {
      FindMergedConditions(BOp, Succ0MBB, Succ1MBB, CurMBB, BOp->getOpcode());
      // If the compares in later blocks need to use values not currently
      // exported from this block, export them now.  This block should always
      // be the first entry.
      assert(SwitchCases[0].ThisBB == CurMBB && "Unexpected lowering!");
      
      // Allow some cases to be rejected.
      if (ShouldEmitAsBranches(SwitchCases)) {
        for (unsigned i = 1, e = SwitchCases.size(); i != e; ++i) {
          ExportFromCurrentBlock(SwitchCases[i].CmpLHS);
          ExportFromCurrentBlock(SwitchCases[i].CmpRHS);
        }
        
        // Emit the branch for this block.
        visitSwitchCase(SwitchCases[0]);
        SwitchCases.erase(SwitchCases.begin());
        return;
      }
      
      // Okay, we decided not to do this, remove any inserted MBB's and clear
      // SwitchCases.
      for (unsigned i = 1, e = SwitchCases.size(); i != e; ++i)
        CurMBB->getParent()->getBasicBlockList().erase(SwitchCases[i].ThisBB);
      
      SwitchCases.clear();
    }
  }
  
  // Create a CaseBlock record representing this branch.
  SelectionDAGISel::CaseBlock CB(ISD::SETEQ, CondVal, ConstantBool::getTrue(),
                                 Succ0MBB, Succ1MBB, CurMBB);
  // Use visitSwitchCase to actually insert the fast branch sequence for this
  // cond branch.
  visitSwitchCase(CB);
}

/// visitSwitchCase - Emits the necessary code to represent a single node in
/// the binary search tree resulting from lowering a switch instruction.
void SelectionDAGLowering::visitSwitchCase(SelectionDAGISel::CaseBlock &CB) {
  SDOperand Cond;
  SDOperand CondLHS = getValue(CB.CmpLHS);
  
  // Build the setcc now, fold "(X == true)" to X and "(X == false)" to !X to
  // handle common cases produced by branch lowering.
  if (CB.CmpRHS == ConstantBool::getTrue() && CB.CC == ISD::SETEQ)
    Cond = CondLHS;
  else if (CB.CmpRHS == ConstantBool::getFalse() && CB.CC == ISD::SETEQ) {
    SDOperand True = DAG.getConstant(1, CondLHS.getValueType());
    Cond = DAG.getNode(ISD::XOR, CondLHS.getValueType(), CondLHS, True);
  } else
    Cond = DAG.getSetCC(MVT::i1, CondLHS, getValue(CB.CmpRHS), CB.CC);
  
  // Set NextBlock to be the MBB immediately after the current one, if any.
  // This is used to avoid emitting unnecessary branches to the next block.
  MachineBasicBlock *NextBlock = 0;
  MachineFunction::iterator BBI = CurMBB;
  if (++BBI != CurMBB->getParent()->end())
    NextBlock = BBI;
  
  // If the lhs block is the next block, invert the condition so that we can
  // fall through to the lhs instead of the rhs block.
  if (CB.TrueBB == NextBlock) {
    std::swap(CB.TrueBB, CB.FalseBB);
    SDOperand True = DAG.getConstant(1, Cond.getValueType());
    Cond = DAG.getNode(ISD::XOR, Cond.getValueType(), Cond, True);
  }
  SDOperand BrCond = DAG.getNode(ISD::BRCOND, MVT::Other, getRoot(), Cond,
                                 DAG.getBasicBlock(CB.TrueBB));
  if (CB.FalseBB == NextBlock)
    DAG.setRoot(BrCond);
  else
    DAG.setRoot(DAG.getNode(ISD::BR, MVT::Other, BrCond, 
                            DAG.getBasicBlock(CB.FalseBB)));
  // Update successor info
  CurMBB->addSuccessor(CB.TrueBB);
  CurMBB->addSuccessor(CB.FalseBB);
}

void SelectionDAGLowering::visitJumpTable(SelectionDAGISel::JumpTable &JT) {
  // Emit the code for the jump table
  MVT::ValueType PTy = TLI.getPointerTy();
  SDOperand Index = DAG.getCopyFromReg(getRoot(), JT.Reg, PTy);
  SDOperand Table = DAG.getJumpTable(JT.JTI, PTy);
  DAG.setRoot(DAG.getNode(ISD::BR_JT, MVT::Other, Index.getValue(1),
                          Table, Index));
  return;
}

void SelectionDAGLowering::visitSwitch(SwitchInst &I) {
  // Figure out which block is immediately after the current one.
  MachineBasicBlock *NextBlock = 0;
  MachineFunction::iterator BBI = CurMBB;

  if (++BBI != CurMBB->getParent()->end())
    NextBlock = BBI;
  
  MachineBasicBlock *Default = FuncInfo.MBBMap[I.getDefaultDest()];

  // If there is only the default destination, branch to it if it is not the
  // next basic block.  Otherwise, just fall through.
  if (I.getNumOperands() == 2) {
    // Update machine-CFG edges.

    // If this is not a fall-through branch, emit the branch.
    if (Default != NextBlock)
      DAG.setRoot(DAG.getNode(ISD::BR, MVT::Other, getRoot(),
                              DAG.getBasicBlock(Default)));

    CurMBB->addSuccessor(Default);
    return;
  }
  
  // If there are any non-default case statements, create a vector of Cases
  // representing each one, and sort the vector so that we can efficiently
  // create a binary search tree from them.
  std::vector<Case> Cases;

  for (unsigned i = 1; i < I.getNumSuccessors(); ++i) {
    MachineBasicBlock *SMBB = FuncInfo.MBBMap[I.getSuccessor(i)];
    Cases.push_back(Case(I.getSuccessorValue(i), SMBB));
  }

  std::sort(Cases.begin(), Cases.end(), CaseCmp());
  
  // Get the Value to be switched on and default basic blocks, which will be
  // inserted into CaseBlock records, representing basic blocks in the binary
  // search tree.
  Value *SV = I.getOperand(0);

  // Get the MachineFunction which holds the current MBB.  This is used during
  // emission of jump tables, and when inserting any additional MBBs necessary
  // to represent the switch.
  MachineFunction *CurMF = CurMBB->getParent();
  const BasicBlock *LLVMBB = CurMBB->getBasicBlock();
  
  // If the switch has few cases (two or less) emit a series of specific
  // tests.
  if (Cases.size() < 3) {
    // TODO: If any two of the cases has the same destination, and if one value
    // is the same as the other, but has one bit unset that the other has set,
    // use bit manipulation to do two compares at once.  For example:
    // "if (X == 6 || X == 4)" -> "if ((X|2) == 6)"
    
    // Rearrange the case blocks so that the last one falls through if possible.
    if (NextBlock && Default != NextBlock && Cases.back().second != NextBlock) {
      // The last case block won't fall through into 'NextBlock' if we emit the
      // branches in this order.  See if rearranging a case value would help.
      for (unsigned i = 0, e = Cases.size()-1; i != e; ++i) {
        if (Cases[i].second == NextBlock) {
          std::swap(Cases[i], Cases.back());
          break;
        }
      }
    }
    
    // Create a CaseBlock record representing a conditional branch to
    // the Case's target mbb if the value being switched on SV is equal
    // to C.
    MachineBasicBlock *CurBlock = CurMBB;
    for (unsigned i = 0, e = Cases.size(); i != e; ++i) {
      MachineBasicBlock *FallThrough;
      if (i != e-1) {
        FallThrough = new MachineBasicBlock(CurMBB->getBasicBlock());
        CurMF->getBasicBlockList().insert(BBI, FallThrough);
      } else {
        // If the last case doesn't match, go to the default block.
        FallThrough = Default;
      }
      
      SelectionDAGISel::CaseBlock CB(ISD::SETEQ, SV, Cases[i].first,
                                     Cases[i].second, FallThrough, CurBlock);
    
      // If emitting the first comparison, just call visitSwitchCase to emit the
      // code into the current block.  Otherwise, push the CaseBlock onto the
      // vector to be later processed by SDISel, and insert the node's MBB
      // before the next MBB.
      if (CurBlock == CurMBB)
        visitSwitchCase(CB);
      else
        SwitchCases.push_back(CB);
      
      CurBlock = FallThrough;
    }
    return;
  }

  // If the switch has more than 5 blocks, and at least 31.25% dense, and the 
  // target supports indirect branches, then emit a jump table rather than 
  // lowering the switch to a binary tree of conditional branches.
  if ((TLI.isOperationLegal(ISD::BR_JT, MVT::Other) ||
       TLI.isOperationLegal(ISD::BRIND, MVT::Other)) &&
      Cases.size() > 5) {
    uint64_t First =cast<ConstantIntegral>(Cases.front().first)->getZExtValue();
    uint64_t Last  = cast<ConstantIntegral>(Cases.back().first)->getZExtValue();
    double Density = (double)Cases.size() / (double)((Last - First) + 1ULL);
    
    if (Density >= 0.3125) {
      // Create a new basic block to hold the code for loading the address
      // of the jump table, and jumping to it.  Update successor information;
      // we will either branch to the default case for the switch, or the jump
      // table.
      MachineBasicBlock *JumpTableBB = new MachineBasicBlock(LLVMBB);
      CurMF->getBasicBlockList().insert(BBI, JumpTableBB);
      CurMBB->addSuccessor(Default);
      CurMBB->addSuccessor(JumpTableBB);
      
      // Subtract the lowest switch case value from the value being switched on
      // and conditional branch to default mbb if the result is greater than the
      // difference between smallest and largest cases.
      SDOperand SwitchOp = getValue(SV);
      MVT::ValueType VT = SwitchOp.getValueType();
      SDOperand SUB = DAG.getNode(ISD::SUB, VT, SwitchOp, 
                                  DAG.getConstant(First, VT));

      // The SDNode we just created, which holds the value being switched on
      // minus the the smallest case value, needs to be copied to a virtual
      // register so it can be used as an index into the jump table in a 
      // subsequent basic block.  This value may be smaller or larger than the
      // target's pointer type, and therefore require extension or truncating.
      if (VT > TLI.getPointerTy())
        SwitchOp = DAG.getNode(ISD::TRUNCATE, TLI.getPointerTy(), SUB);
      else
        SwitchOp = DAG.getNode(ISD::ZERO_EXTEND, TLI.getPointerTy(), SUB);

      unsigned JumpTableReg = FuncInfo.MakeReg(TLI.getPointerTy());
      SDOperand CopyTo = DAG.getCopyToReg(getRoot(), JumpTableReg, SwitchOp);
      
      // Emit the range check for the jump table, and branch to the default
      // block for the switch statement if the value being switched on exceeds
      // the largest case in the switch.
      SDOperand CMP = DAG.getSetCC(TLI.getSetCCResultTy(), SUB,
                                   DAG.getConstant(Last-First,VT), ISD::SETUGT);
      DAG.setRoot(DAG.getNode(ISD::BRCOND, MVT::Other, CopyTo, CMP, 
                              DAG.getBasicBlock(Default)));

      // Build a vector of destination BBs, corresponding to each target
      // of the jump table.  If the value of the jump table slot corresponds to
      // a case statement, push the case's BB onto the vector, otherwise, push
      // the default BB.
      std::vector<MachineBasicBlock*> DestBBs;
      uint64_t TEI = First;
      for (CaseItr ii = Cases.begin(), ee = Cases.end(); ii != ee; ++TEI)
        if (cast<ConstantIntegral>(ii->first)->getZExtValue() == TEI) {
          DestBBs.push_back(ii->second);
          ++ii;
        } else {
          DestBBs.push_back(Default);
        }
      
      // Update successor info.  Add one edge to each unique successor.
      // Vector bool would be better, but vector<bool> is really slow.
      std::vector<unsigned char> SuccsHandled;
      SuccsHandled.resize(CurMBB->getParent()->getNumBlockIDs());
      
      for (std::vector<MachineBasicBlock*>::iterator I = DestBBs.begin(), 
           E = DestBBs.end(); I != E; ++I) {
        if (!SuccsHandled[(*I)->getNumber()]) {
          SuccsHandled[(*I)->getNumber()] = true;
          JumpTableBB->addSuccessor(*I);
        }
      }
      
      // Create a jump table index for this jump table, or return an existing
      // one.
      unsigned JTI = CurMF->getJumpTableInfo()->getJumpTableIndex(DestBBs);
      
      // Set the jump table information so that we can codegen it as a second
      // MachineBasicBlock
      JT.Reg = JumpTableReg;
      JT.JTI = JTI;
      JT.MBB = JumpTableBB;
      JT.Default = Default;
      return;
    }
  }
  
  // Push the initial CaseRec onto the worklist
  std::vector<CaseRec> CaseVec;
  CaseVec.push_back(CaseRec(CurMBB,0,0,CaseRange(Cases.begin(),Cases.end())));
  
  while (!CaseVec.empty()) {
    // Grab a record representing a case range to process off the worklist
    CaseRec CR = CaseVec.back();
    CaseVec.pop_back();
    
    // Size is the number of Cases represented by this range.  If Size is 1,
    // then we are processing a leaf of the binary search tree.  Otherwise,
    // we need to pick a pivot, and push left and right ranges onto the 
    // worklist.
    unsigned Size = CR.Range.second - CR.Range.first;
    
    if (Size == 1) {
      // Create a CaseBlock record representing a conditional branch to
      // the Case's target mbb if the value being switched on SV is equal
      // to C.  Otherwise, branch to default.
      Constant *C = CR.Range.first->first;
      MachineBasicBlock *Target = CR.Range.first->second;
      SelectionDAGISel::CaseBlock CB(ISD::SETEQ, SV, C, Target, Default, 
                                     CR.CaseBB);

      // If the MBB representing the leaf node is the current MBB, then just
      // call visitSwitchCase to emit the code into the current block.
      // Otherwise, push the CaseBlock onto the vector to be later processed
      // by SDISel, and insert the node's MBB before the next MBB.
      if (CR.CaseBB == CurMBB)
        visitSwitchCase(CB);
      else
        SwitchCases.push_back(CB);
    } else {
      // split case range at pivot
      CaseItr Pivot = CR.Range.first + (Size / 2);
      CaseRange LHSR(CR.Range.first, Pivot);
      CaseRange RHSR(Pivot, CR.Range.second);
      Constant *C = Pivot->first;
      MachineBasicBlock *FalseBB = 0, *TrueBB = 0;

      // We know that we branch to the LHS if the Value being switched on is
      // less than the Pivot value, C.  We use this to optimize our binary 
      // tree a bit, by recognizing that if SV is greater than or equal to the
      // LHS's Case Value, and that Case Value is exactly one less than the 
      // Pivot's Value, then we can branch directly to the LHS's Target,
      // rather than creating a leaf node for it.
      if ((LHSR.second - LHSR.first) == 1 &&
          LHSR.first->first == CR.GE &&
          cast<ConstantIntegral>(C)->getZExtValue() ==
          (cast<ConstantIntegral>(CR.GE)->getZExtValue() + 1ULL)) {
        TrueBB = LHSR.first->second;
      } else {
        TrueBB = new MachineBasicBlock(LLVMBB);
        CurMF->getBasicBlockList().insert(BBI, TrueBB);
        CaseVec.push_back(CaseRec(TrueBB, C, CR.GE, LHSR));
      }

      // Similar to the optimization above, if the Value being switched on is
      // known to be less than the Constant CR.LT, and the current Case Value
      // is CR.LT - 1, then we can branch directly to the target block for
      // the current Case Value, rather than emitting a RHS leaf node for it.
      if ((RHSR.second - RHSR.first) == 1 && CR.LT &&
          cast<ConstantIntegral>(RHSR.first->first)->getZExtValue() ==
          (cast<ConstantIntegral>(CR.LT)->getZExtValue() - 1ULL)) {
        FalseBB = RHSR.first->second;
      } else {
        FalseBB = new MachineBasicBlock(LLVMBB);
        CurMF->getBasicBlockList().insert(BBI, FalseBB);
        CaseVec.push_back(CaseRec(FalseBB,CR.LT,C,RHSR));
      }

      // Create a CaseBlock record representing a conditional branch to
      // the LHS node if the value being switched on SV is less than C. 
      // Otherwise, branch to LHS.
      ISD::CondCode CC = C->getType()->isSigned() ? ISD::SETLT : ISD::SETULT;
      SelectionDAGISel::CaseBlock CB(CC, SV, C, TrueBB, FalseBB, CR.CaseBB);

      if (CR.CaseBB == CurMBB)
        visitSwitchCase(CB);
      else
        SwitchCases.push_back(CB);
    }
  }
}

void SelectionDAGLowering::visitSub(User &I) {
  // -0.0 - X --> fneg
  if (I.getType()->isFloatingPoint()) {
    if (ConstantFP *CFP = dyn_cast<ConstantFP>(I.getOperand(0)))
      if (CFP->isExactlyValue(-0.0)) {
        SDOperand Op2 = getValue(I.getOperand(1));
        setValue(&I, DAG.getNode(ISD::FNEG, Op2.getValueType(), Op2));
        return;
      }
    visitFPBinary(I, ISD::FSUB, ISD::VSUB);
  } else 
    visitIntBinary(I, ISD::SUB, ISD::VSUB);
}

void 
SelectionDAGLowering::visitIntBinary(User &I, unsigned IntOp, unsigned VecOp) {
  const Type *Ty = I.getType();
  SDOperand Op1 = getValue(I.getOperand(0));
  SDOperand Op2 = getValue(I.getOperand(1));

  if (const PackedType *PTy = dyn_cast<PackedType>(Ty)) {
    SDOperand Num = DAG.getConstant(PTy->getNumElements(), MVT::i32);
    SDOperand Typ = DAG.getValueType(TLI.getValueType(PTy->getElementType()));
    setValue(&I, DAG.getNode(VecOp, MVT::Vector, Op1, Op2, Num, Typ));
  } else {
    setValue(&I, DAG.getNode(IntOp, Op1.getValueType(), Op1, Op2));
  }
}

void 
SelectionDAGLowering::visitFPBinary(User &I, unsigned FPOp, unsigned VecOp) {
  const Type *Ty = I.getType();
  SDOperand Op1 = getValue(I.getOperand(0));
  SDOperand Op2 = getValue(I.getOperand(1));

  if (const PackedType *PTy = dyn_cast<PackedType>(Ty)) {
    SDOperand Num = DAG.getConstant(PTy->getNumElements(), MVT::i32);
    SDOperand Typ = DAG.getValueType(TLI.getValueType(PTy->getElementType()));
    setValue(&I, DAG.getNode(VecOp, MVT::Vector, Op1, Op2, Num, Typ));
  } else {
    setValue(&I, DAG.getNode(FPOp, Op1.getValueType(), Op1, Op2));
  }
}

void SelectionDAGLowering::visitShift(User &I, unsigned Opcode) {
  SDOperand Op1 = getValue(I.getOperand(0));
  SDOperand Op2 = getValue(I.getOperand(1));
  
  Op2 = DAG.getNode(ISD::ANY_EXTEND, TLI.getShiftAmountTy(), Op2);
  
  setValue(&I, DAG.getNode(Opcode, Op1.getValueType(), Op1, Op2));
}

void SelectionDAGLowering::visitICmp(User &I) {
  ICmpInst *IC = cast<ICmpInst>(&I);
  SDOperand Op1 = getValue(IC->getOperand(0));
  SDOperand Op2 = getValue(IC->getOperand(1));
  ISD::CondCode Opcode;
  switch (IC->getPredicate()) {
    case ICmpInst::ICMP_EQ  : Opcode = ISD::SETEQ; break;
    case ICmpInst::ICMP_NE  : Opcode = ISD::SETNE; break;
    case ICmpInst::ICMP_UGT : Opcode = ISD::SETUGT; break;
    case ICmpInst::ICMP_UGE : Opcode = ISD::SETUGE; break;
    case ICmpInst::ICMP_ULT : Opcode = ISD::SETULT; break;
    case ICmpInst::ICMP_ULE : Opcode = ISD::SETULE; break;
    case ICmpInst::ICMP_SGT : Opcode = ISD::SETGT; break;
    case ICmpInst::ICMP_SGE : Opcode = ISD::SETGE; break;
    case ICmpInst::ICMP_SLT : Opcode = ISD::SETLT; break;
    case ICmpInst::ICMP_SLE : Opcode = ISD::SETLE; break;
    default:
      assert(!"Invalid ICmp predicate value");
      Opcode = ISD::SETEQ;
      break;
  }
  setValue(&I, DAG.getSetCC(MVT::i1, Op1, Op2, Opcode));
}

void SelectionDAGLowering::visitFCmp(User &I) {
  FCmpInst *FC = cast<FCmpInst>(&I);
  SDOperand Op1 = getValue(FC->getOperand(0));
  SDOperand Op2 = getValue(FC->getOperand(1));
  ISD::CondCode Opcode;
  switch (FC->getPredicate()) {
    case FCmpInst::FCMP_FALSE : Opcode = ISD::SETFALSE;
    case FCmpInst::FCMP_OEQ   : Opcode = ISD::SETOEQ;
    case FCmpInst::FCMP_OGT   : Opcode = ISD::SETOGT;
    case FCmpInst::FCMP_OGE   : Opcode = ISD::SETOGE;
    case FCmpInst::FCMP_OLT   : Opcode = ISD::SETOLT;
    case FCmpInst::FCMP_OLE   : Opcode = ISD::SETOLE;
    case FCmpInst::FCMP_ONE   : Opcode = ISD::SETONE;
    case FCmpInst::FCMP_ORD   : Opcode = ISD::SETO;
    case FCmpInst::FCMP_UNO   : Opcode = ISD::SETUO;
    case FCmpInst::FCMP_UEQ   : Opcode = ISD::SETUEQ;
    case FCmpInst::FCMP_UGT   : Opcode = ISD::SETUGT;
    case FCmpInst::FCMP_UGE   : Opcode = ISD::SETUGE;
    case FCmpInst::FCMP_ULT   : Opcode = ISD::SETULT;
    case FCmpInst::FCMP_ULE   : Opcode = ISD::SETULE;
    case FCmpInst::FCMP_UNE   : Opcode = ISD::SETUNE;
    case FCmpInst::FCMP_TRUE  : Opcode = ISD::SETTRUE;
    default:
      assert(!"Invalid FCmp predicate value");
      Opcode = ISD::SETFALSE;
      break;
  }
  setValue(&I, DAG.getSetCC(MVT::i1, Op1, Op2, Opcode));
}

void SelectionDAGLowering::visitSetCC(User &I,ISD::CondCode SignedOpcode,
                                      ISD::CondCode UnsignedOpcode,
                                      ISD::CondCode FPOpcode) {
  SDOperand Op1 = getValue(I.getOperand(0));
  SDOperand Op2 = getValue(I.getOperand(1));
  ISD::CondCode Opcode = SignedOpcode;
  if (!FiniteOnlyFPMath() && I.getOperand(0)->getType()->isFloatingPoint())
    Opcode = FPOpcode;
  else if (I.getOperand(0)->getType()->isUnsigned())
    Opcode = UnsignedOpcode;
  setValue(&I, DAG.getSetCC(MVT::i1, Op1, Op2, Opcode));
}

void SelectionDAGLowering::visitSelect(User &I) {
  SDOperand Cond     = getValue(I.getOperand(0));
  SDOperand TrueVal  = getValue(I.getOperand(1));
  SDOperand FalseVal = getValue(I.getOperand(2));
  if (!isa<PackedType>(I.getType())) {
    setValue(&I, DAG.getNode(ISD::SELECT, TrueVal.getValueType(), Cond,
                             TrueVal, FalseVal));
  } else {
    setValue(&I, DAG.getNode(ISD::VSELECT, MVT::Vector, Cond, TrueVal, FalseVal,
                             *(TrueVal.Val->op_end()-2),
                             *(TrueVal.Val->op_end()-1)));
  }
}


void SelectionDAGLowering::visitTrunc(User &I) {
  // TruncInst cannot be a no-op cast because sizeof(src) > sizeof(dest).
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::TRUNCATE, DestVT, N));
}

void SelectionDAGLowering::visitZExt(User &I) {
  // ZExt cannot be a no-op cast because sizeof(src) < sizeof(dest).
  // ZExt also can't be a cast to bool for same reason. So, nothing much to do
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::ZERO_EXTEND, DestVT, N));
}

void SelectionDAGLowering::visitSExt(User &I) {
  // SExt cannot be a no-op cast because sizeof(src) < sizeof(dest).
  // SExt also can't be a cast to bool for same reason. So, nothing much to do
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::SIGN_EXTEND, DestVT, N));
}

void SelectionDAGLowering::visitFPTrunc(User &I) {
  // FPTrunc is never a no-op cast, no need to check
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::FP_ROUND, DestVT, N));
}

void SelectionDAGLowering::visitFPExt(User &I){ 
  // FPTrunc is never a no-op cast, no need to check
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::FP_EXTEND, DestVT, N));
}

void SelectionDAGLowering::visitFPToUI(User &I) { 
  // FPToUI is never a no-op cast, no need to check
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::FP_TO_UINT, DestVT, N));
}

void SelectionDAGLowering::visitFPToSI(User &I) {
  // FPToSI is never a no-op cast, no need to check
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::FP_TO_SINT, DestVT, N));
}

void SelectionDAGLowering::visitUIToFP(User &I) { 
  // UIToFP is never a no-op cast, no need to check
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::UINT_TO_FP, DestVT, N));
}

void SelectionDAGLowering::visitSIToFP(User &I){ 
  // UIToFP is never a no-op cast, no need to check
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  setValue(&I, DAG.getNode(ISD::SINT_TO_FP, DestVT, N));
}

void SelectionDAGLowering::visitPtrToInt(User &I) {
  // What to do depends on the size of the integer and the size of the pointer.
  // We can either truncate, zero extend, or no-op, accordingly.
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType SrcVT = N.getValueType();
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  SDOperand Result;
  if (MVT::getSizeInBits(DestVT) < MVT::getSizeInBits(SrcVT))
    Result = DAG.getNode(ISD::TRUNCATE, DestVT, N);
  else 
    // Note: ZERO_EXTEND can handle cases where the sizes are equal too
    Result = DAG.getNode(ISD::ZERO_EXTEND, DestVT, N);
  setValue(&I, Result);
}

void SelectionDAGLowering::visitIntToPtr(User &I) {
  // What to do depends on the size of the integer and the size of the pointer.
  // We can either truncate, zero extend, or no-op, accordingly.
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType SrcVT = N.getValueType();
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  if (MVT::getSizeInBits(DestVT) < MVT::getSizeInBits(SrcVT))
    setValue(&I, DAG.getNode(ISD::TRUNCATE, DestVT, N));
  else 
    // Note: ZERO_EXTEND can handle cases where the sizes are equal too
    setValue(&I, DAG.getNode(ISD::ZERO_EXTEND, DestVT, N));
}

void SelectionDAGLowering::visitBitCast(User &I) { 
  SDOperand N = getValue(I.getOperand(0));
  MVT::ValueType DestVT = TLI.getValueType(I.getType());
  if (DestVT == MVT::Vector) {
    // This is a cast to a vector from something else.  
    // Get information about the output vector.
    const PackedType *DestTy = cast<PackedType>(I.getType());
    MVT::ValueType EltVT = TLI.getValueType(DestTy->getElementType());
    setValue(&I, DAG.getNode(ISD::VBIT_CONVERT, DestVT, N, 
                             DAG.getConstant(DestTy->getNumElements(),MVT::i32),
                             DAG.getValueType(EltVT)));
    return;
  } 
  MVT::ValueType SrcVT = N.getValueType();
  if (SrcVT == MVT::Vector) {
    // This is a cast from a vctor to something else. 
    // Get information about the input vector.
    setValue(&I, DAG.getNode(ISD::VBIT_CONVERT, DestVT, N));
    return;
  }

  // BitCast assures us that source and destination are the same size so this 
  // is either a BIT_CONVERT or a no-op.
  if (DestVT != N.getValueType())
    setValue(&I, DAG.getNode(ISD::BIT_CONVERT, DestVT, N)); // convert types
  else
    setValue(&I, N); // noop cast.
}

void SelectionDAGLowering::visitInsertElement(User &I) {
  SDOperand InVec = getValue(I.getOperand(0));
  SDOperand InVal = getValue(I.getOperand(1));
  SDOperand InIdx = DAG.getNode(ISD::ZERO_EXTEND, TLI.getPointerTy(),
                                getValue(I.getOperand(2)));

  SDOperand Num = *(InVec.Val->op_end()-2);
  SDOperand Typ = *(InVec.Val->op_end()-1);
  setValue(&I, DAG.getNode(ISD::VINSERT_VECTOR_ELT, MVT::Vector,
                           InVec, InVal, InIdx, Num, Typ));
}

void SelectionDAGLowering::visitExtractElement(User &I) {
  SDOperand InVec = getValue(I.getOperand(0));
  SDOperand InIdx = DAG.getNode(ISD::ZERO_EXTEND, TLI.getPointerTy(),
                                getValue(I.getOperand(1)));
  SDOperand Typ = *(InVec.Val->op_end()-1);
  setValue(&I, DAG.getNode(ISD::VEXTRACT_VECTOR_ELT,
                           TLI.getValueType(I.getType()), InVec, InIdx));
}

void SelectionDAGLowering::visitShuffleVector(User &I) {
  SDOperand V1   = getValue(I.getOperand(0));
  SDOperand V2   = getValue(I.getOperand(1));
  SDOperand Mask = getValue(I.getOperand(2));

  SDOperand Num = *(V1.Val->op_end()-2);
  SDOperand Typ = *(V2.Val->op_end()-1);
  setValue(&I, DAG.getNode(ISD::VVECTOR_SHUFFLE, MVT::Vector,
                           V1, V2, Mask, Num, Typ));
}


void SelectionDAGLowering::visitGetElementPtr(User &I) {
  SDOperand N = getValue(I.getOperand(0));
  const Type *Ty = I.getOperand(0)->getType();

  for (GetElementPtrInst::op_iterator OI = I.op_begin()+1, E = I.op_end();
       OI != E; ++OI) {
    Value *Idx = *OI;
    if (const StructType *StTy = dyn_cast<StructType>(Ty)) {
      unsigned Field = cast<ConstantInt>(Idx)->getZExtValue();
      if (Field) {
        // N = N + Offset
        uint64_t Offset = TD->getStructLayout(StTy)->MemberOffsets[Field];
        N = DAG.getNode(ISD::ADD, N.getValueType(), N,
                        getIntPtrConstant(Offset));
      }
      Ty = StTy->getElementType(Field);
    } else {
      Ty = cast<SequentialType>(Ty)->getElementType();

      // If this is a constant subscript, handle it quickly.
      if (ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
        if (CI->getZExtValue() == 0) continue;
        uint64_t Offs;
        if (CI->getType()->isSigned()) 
          Offs = (int64_t)
            TD->getTypeSize(Ty)*cast<ConstantInt>(CI)->getSExtValue();
        else
          Offs = 
            TD->getTypeSize(Ty)*cast<ConstantInt>(CI)->getZExtValue();
        N = DAG.getNode(ISD::ADD, N.getValueType(), N, getIntPtrConstant(Offs));
        continue;
      }
      
      // N = N + Idx * ElementSize;
      uint64_t ElementSize = TD->getTypeSize(Ty);
      SDOperand IdxN = getValue(Idx);

      // If the index is smaller or larger than intptr_t, truncate or extend
      // it.
      if (IdxN.getValueType() < N.getValueType()) {
        if (Idx->getType()->isSigned())
          IdxN = DAG.getNode(ISD::SIGN_EXTEND, N.getValueType(), IdxN);
        else
          IdxN = DAG.getNode(ISD::ZERO_EXTEND, N.getValueType(), IdxN);
      } else if (IdxN.getValueType() > N.getValueType())
        IdxN = DAG.getNode(ISD::TRUNCATE, N.getValueType(), IdxN);

      // If this is a multiply by a power of two, turn it into a shl
      // immediately.  This is a very common case.
      if (isPowerOf2_64(ElementSize)) {
        unsigned Amt = Log2_64(ElementSize);
        IdxN = DAG.getNode(ISD::SHL, N.getValueType(), IdxN,
                           DAG.getConstant(Amt, TLI.getShiftAmountTy()));
        N = DAG.getNode(ISD::ADD, N.getValueType(), N, IdxN);
        continue;
      }
      
      SDOperand Scale = getIntPtrConstant(ElementSize);
      IdxN = DAG.getNode(ISD::MUL, N.getValueType(), IdxN, Scale);
      N = DAG.getNode(ISD::ADD, N.getValueType(), N, IdxN);
    }
  }
  setValue(&I, N);
}

void SelectionDAGLowering::visitAlloca(AllocaInst &I) {
  // If this is a fixed sized alloca in the entry block of the function,
  // allocate it statically on the stack.
  if (FuncInfo.StaticAllocaMap.count(&I))
    return;   // getValue will auto-populate this.

  const Type *Ty = I.getAllocatedType();
  uint64_t TySize = TLI.getTargetData()->getTypeSize(Ty);
  unsigned Align = std::max((unsigned)TLI.getTargetData()->getTypeAlignment(Ty),
                            I.getAlignment());

  SDOperand AllocSize = getValue(I.getArraySize());
  MVT::ValueType IntPtr = TLI.getPointerTy();
  if (IntPtr < AllocSize.getValueType())
    AllocSize = DAG.getNode(ISD::TRUNCATE, IntPtr, AllocSize);
  else if (IntPtr > AllocSize.getValueType())
    AllocSize = DAG.getNode(ISD::ZERO_EXTEND, IntPtr, AllocSize);

  AllocSize = DAG.getNode(ISD::MUL, IntPtr, AllocSize,
                          getIntPtrConstant(TySize));

  // Handle alignment.  If the requested alignment is less than or equal to the
  // stack alignment, ignore it and round the size of the allocation up to the
  // stack alignment size.  If the size is greater than the stack alignment, we
  // note this in the DYNAMIC_STACKALLOC node.
  unsigned StackAlign =
    TLI.getTargetMachine().getFrameInfo()->getStackAlignment();
  if (Align <= StackAlign) {
    Align = 0;
    // Add SA-1 to the size.
    AllocSize = DAG.getNode(ISD::ADD, AllocSize.getValueType(), AllocSize,
                            getIntPtrConstant(StackAlign-1));
    // Mask out the low bits for alignment purposes.
    AllocSize = DAG.getNode(ISD::AND, AllocSize.getValueType(), AllocSize,
                            getIntPtrConstant(~(uint64_t)(StackAlign-1)));
  }

  SDOperand Ops[] = { getRoot(), AllocSize, getIntPtrConstant(Align) };
  const MVT::ValueType *VTs = DAG.getNodeValueTypes(AllocSize.getValueType(),
                                                    MVT::Other);
  SDOperand DSA = DAG.getNode(ISD::DYNAMIC_STACKALLOC, VTs, 2, Ops, 3);
  DAG.setRoot(setValue(&I, DSA).getValue(1));

  // Inform the Frame Information that we have just allocated a variable-sized
  // object.
  CurMBB->getParent()->getFrameInfo()->CreateVariableSizedObject();
}

void SelectionDAGLowering::visitLoad(LoadInst &I) {
  SDOperand Ptr = getValue(I.getOperand(0));

  SDOperand Root;
  if (I.isVolatile())
    Root = getRoot();
  else {
    // Do not serialize non-volatile loads against each other.
    Root = DAG.getRoot();
  }

  setValue(&I, getLoadFrom(I.getType(), Ptr, I.getOperand(0),
                           Root, I.isVolatile()));
}

SDOperand SelectionDAGLowering::getLoadFrom(const Type *Ty, SDOperand Ptr,
                                            const Value *SV, SDOperand Root,
                                            bool isVolatile) {
  SDOperand L;
  if (const PackedType *PTy = dyn_cast<PackedType>(Ty)) {
    MVT::ValueType PVT = TLI.getValueType(PTy->getElementType());
    L = DAG.getVecLoad(PTy->getNumElements(), PVT, Root, Ptr,
                       DAG.getSrcValue(SV));
  } else {
    L = DAG.getLoad(TLI.getValueType(Ty), Root, Ptr, SV, 0, isVolatile);
  }

  if (isVolatile)
    DAG.setRoot(L.getValue(1));
  else
    PendingLoads.push_back(L.getValue(1));
  
  return L;
}


void SelectionDAGLowering::visitStore(StoreInst &I) {
  Value *SrcV = I.getOperand(0);
  SDOperand Src = getValue(SrcV);
  SDOperand Ptr = getValue(I.getOperand(1));
  DAG.setRoot(DAG.getStore(getRoot(), Src, Ptr, I.getOperand(1), 0,
                           I.isVolatile()));
}

/// IntrinsicCannotAccessMemory - Return true if the specified intrinsic cannot
/// access memory and has no other side effects at all.
static bool IntrinsicCannotAccessMemory(unsigned IntrinsicID) {
#define GET_NO_MEMORY_INTRINSICS
#include "llvm/Intrinsics.gen"
#undef GET_NO_MEMORY_INTRINSICS
  return false;
}

// IntrinsicOnlyReadsMemory - Return true if the specified intrinsic doesn't
// have any side-effects or if it only reads memory.
static bool IntrinsicOnlyReadsMemory(unsigned IntrinsicID) {
#define GET_SIDE_EFFECT_INFO
#include "llvm/Intrinsics.gen"
#undef GET_SIDE_EFFECT_INFO
  return false;
}

/// visitTargetIntrinsic - Lower a call of a target intrinsic to an INTRINSIC
/// node.
void SelectionDAGLowering::visitTargetIntrinsic(CallInst &I, 
                                                unsigned Intrinsic) {
  bool HasChain = !IntrinsicCannotAccessMemory(Intrinsic);
  bool OnlyLoad = HasChain && IntrinsicOnlyReadsMemory(Intrinsic);
  
  // Build the operand list.
  SmallVector<SDOperand, 8> Ops;
  if (HasChain) {  // If this intrinsic has side-effects, chainify it.
    if (OnlyLoad) {
      // We don't need to serialize loads against other loads.
      Ops.push_back(DAG.getRoot());
    } else { 
      Ops.push_back(getRoot());
    }
  }
  
  // Add the intrinsic ID as an integer operand.
  Ops.push_back(DAG.getConstant(Intrinsic, TLI.getPointerTy()));

  // Add all operands of the call to the operand list.
  for (unsigned i = 1, e = I.getNumOperands(); i != e; ++i) {
    SDOperand Op = getValue(I.getOperand(i));
    
    // If this is a vector type, force it to the right packed type.
    if (Op.getValueType() == MVT::Vector) {
      const PackedType *OpTy = cast<PackedType>(I.getOperand(i)->getType());
      MVT::ValueType EltVT = TLI.getValueType(OpTy->getElementType());
      
      MVT::ValueType VVT = MVT::getVectorType(EltVT, OpTy->getNumElements());
      assert(VVT != MVT::Other && "Intrinsic uses a non-legal type?");
      Op = DAG.getNode(ISD::VBIT_CONVERT, VVT, Op);
    }
    
    assert(TLI.isTypeLegal(Op.getValueType()) &&
           "Intrinsic uses a non-legal type?");
    Ops.push_back(Op);
  }

  std::vector<MVT::ValueType> VTs;
  if (I.getType() != Type::VoidTy) {
    MVT::ValueType VT = TLI.getValueType(I.getType());
    if (VT == MVT::Vector) {
      const PackedType *DestTy = cast<PackedType>(I.getType());
      MVT::ValueType EltVT = TLI.getValueType(DestTy->getElementType());
      
      VT = MVT::getVectorType(EltVT, DestTy->getNumElements());
      assert(VT != MVT::Other && "Intrinsic uses a non-legal type?");
    }
    
    assert(TLI.isTypeLegal(VT) && "Intrinsic uses a non-legal type?");
    VTs.push_back(VT);
  }
  if (HasChain)
    VTs.push_back(MVT::Other);

  const MVT::ValueType *VTList = DAG.getNodeValueTypes(VTs);

  // Create the node.
  SDOperand Result;
  if (!HasChain)
    Result = DAG.getNode(ISD::INTRINSIC_WO_CHAIN, VTList, VTs.size(),
                         &Ops[0], Ops.size());
  else if (I.getType() != Type::VoidTy)
    Result = DAG.getNode(ISD::INTRINSIC_W_CHAIN, VTList, VTs.size(),
                         &Ops[0], Ops.size());
  else
    Result = DAG.getNode(ISD::INTRINSIC_VOID, VTList, VTs.size(),
                         &Ops[0], Ops.size());

  if (HasChain) {
    SDOperand Chain = Result.getValue(Result.Val->getNumValues()-1);
    if (OnlyLoad)
      PendingLoads.push_back(Chain);
    else
      DAG.setRoot(Chain);
  }
  if (I.getType() != Type::VoidTy) {
    if (const PackedType *PTy = dyn_cast<PackedType>(I.getType())) {
      MVT::ValueType EVT = TLI.getValueType(PTy->getElementType());
      Result = DAG.getNode(ISD::VBIT_CONVERT, MVT::Vector, Result,
                           DAG.getConstant(PTy->getNumElements(), MVT::i32),
                           DAG.getValueType(EVT));
    } 
    setValue(&I, Result);
  }
}

/// visitIntrinsicCall - Lower the call to the specified intrinsic function.  If
/// we want to emit this as a call to a named external function, return the name
/// otherwise lower it and return null.
const char *
SelectionDAGLowering::visitIntrinsicCall(CallInst &I, unsigned Intrinsic) {
  switch (Intrinsic) {
  default:
    // By default, turn this into a target intrinsic node.
    visitTargetIntrinsic(I, Intrinsic);
    return 0;
  case Intrinsic::vastart:  visitVAStart(I); return 0;
  case Intrinsic::vaend:    visitVAEnd(I); return 0;
  case Intrinsic::vacopy:   visitVACopy(I); return 0;
  case Intrinsic::returnaddress: visitFrameReturnAddress(I, false); return 0;
  case Intrinsic::frameaddress:  visitFrameReturnAddress(I, true); return 0;
  case Intrinsic::setjmp:
    return "_setjmp"+!TLI.usesUnderscoreSetJmp();
    break;
  case Intrinsic::longjmp:
    return "_longjmp"+!TLI.usesUnderscoreLongJmp();
    break;
  case Intrinsic::memcpy_i32:
  case Intrinsic::memcpy_i64:
    visitMemIntrinsic(I, ISD::MEMCPY);
    return 0;
  case Intrinsic::memset_i32:
  case Intrinsic::memset_i64:
    visitMemIntrinsic(I, ISD::MEMSET);
    return 0;
  case Intrinsic::memmove_i32:
  case Intrinsic::memmove_i64:
    visitMemIntrinsic(I, ISD::MEMMOVE);
    return 0;
    
  case Intrinsic::dbg_stoppoint: {
    MachineDebugInfo *DebugInfo = DAG.getMachineDebugInfo();
    DbgStopPointInst &SPI = cast<DbgStopPointInst>(I);
    if (DebugInfo && SPI.getContext() && DebugInfo->Verify(SPI.getContext())) {
      SDOperand Ops[5];

      Ops[0] = getRoot();
      Ops[1] = getValue(SPI.getLineValue());
      Ops[2] = getValue(SPI.getColumnValue());

      DebugInfoDesc *DD = DebugInfo->getDescFor(SPI.getContext());
      assert(DD && "Not a debug information descriptor");
      CompileUnitDesc *CompileUnit = cast<CompileUnitDesc>(DD);
      
      Ops[3] = DAG.getString(CompileUnit->getFileName());
      Ops[4] = DAG.getString(CompileUnit->getDirectory());
      
      DAG.setRoot(DAG.getNode(ISD::LOCATION, MVT::Other, Ops, 5));
    }

    return 0;
  }
  case Intrinsic::dbg_region_start: {
    MachineDebugInfo *DebugInfo = DAG.getMachineDebugInfo();
    DbgRegionStartInst &RSI = cast<DbgRegionStartInst>(I);
    if (DebugInfo && RSI.getContext() && DebugInfo->Verify(RSI.getContext())) {
      unsigned LabelID = DebugInfo->RecordRegionStart(RSI.getContext());
      DAG.setRoot(DAG.getNode(ISD::DEBUG_LABEL, MVT::Other, getRoot(),
                              DAG.getConstant(LabelID, MVT::i32)));
    }

    return 0;
  }
  case Intrinsic::dbg_region_end: {
    MachineDebugInfo *DebugInfo = DAG.getMachineDebugInfo();
    DbgRegionEndInst &REI = cast<DbgRegionEndInst>(I);
    if (DebugInfo && REI.getContext() && DebugInfo->Verify(REI.getContext())) {
      unsigned LabelID = DebugInfo->RecordRegionEnd(REI.getContext());
      DAG.setRoot(DAG.getNode(ISD::DEBUG_LABEL, MVT::Other,
                              getRoot(), DAG.getConstant(LabelID, MVT::i32)));
    }

    return 0;
  }
  case Intrinsic::dbg_func_start: {
    MachineDebugInfo *DebugInfo = DAG.getMachineDebugInfo();
    DbgFuncStartInst &FSI = cast<DbgFuncStartInst>(I);
    if (DebugInfo && FSI.getSubprogram() &&
        DebugInfo->Verify(FSI.getSubprogram())) {
      unsigned LabelID = DebugInfo->RecordRegionStart(FSI.getSubprogram());
      DAG.setRoot(DAG.getNode(ISD::DEBUG_LABEL, MVT::Other,
                  getRoot(), DAG.getConstant(LabelID, MVT::i32)));
    }

    return 0;
  }
  case Intrinsic::dbg_declare: {
    MachineDebugInfo *DebugInfo = DAG.getMachineDebugInfo();
    DbgDeclareInst &DI = cast<DbgDeclareInst>(I);
    if (DebugInfo && DI.getVariable() && DebugInfo->Verify(DI.getVariable())) {
      SDOperand AddressOp  = getValue(DI.getAddress());
      if (FrameIndexSDNode *FI = dyn_cast<FrameIndexSDNode>(AddressOp))
        DebugInfo->RecordVariable(DI.getVariable(), FI->getIndex());
    }

    return 0;
  }
    
  case Intrinsic::isunordered_f32:
  case Intrinsic::isunordered_f64:
    setValue(&I, DAG.getSetCC(MVT::i1,getValue(I.getOperand(1)),
                              getValue(I.getOperand(2)), ISD::SETUO));
    return 0;
    
  case Intrinsic::sqrt_f32:
  case Intrinsic::sqrt_f64:
    setValue(&I, DAG.getNode(ISD::FSQRT,
                             getValue(I.getOperand(1)).getValueType(),
                             getValue(I.getOperand(1))));
    return 0;
  case Intrinsic::powi_f32:
  case Intrinsic::powi_f64:
    setValue(&I, DAG.getNode(ISD::FPOWI,
                             getValue(I.getOperand(1)).getValueType(),
                             getValue(I.getOperand(1)),
                             getValue(I.getOperand(2))));
    return 0;
  case Intrinsic::pcmarker: {
    SDOperand Tmp = getValue(I.getOperand(1));
    DAG.setRoot(DAG.getNode(ISD::PCMARKER, MVT::Other, getRoot(), Tmp));
    return 0;
  }
  case Intrinsic::readcyclecounter: {
    SDOperand Op = getRoot();
    SDOperand Tmp = DAG.getNode(ISD::READCYCLECOUNTER,
                                DAG.getNodeValueTypes(MVT::i64, MVT::Other), 2,
                                &Op, 1);
    setValue(&I, Tmp);
    DAG.setRoot(Tmp.getValue(1));
    return 0;
  }
  case Intrinsic::bswap_i16:
  case Intrinsic::bswap_i32:
  case Intrinsic::bswap_i64:
    setValue(&I, DAG.getNode(ISD::BSWAP,
                             getValue(I.getOperand(1)).getValueType(),
                             getValue(I.getOperand(1))));
    return 0;
  case Intrinsic::cttz_i8:
  case Intrinsic::cttz_i16:
  case Intrinsic::cttz_i32:
  case Intrinsic::cttz_i64:
    setValue(&I, DAG.getNode(ISD::CTTZ,
                             getValue(I.getOperand(1)).getValueType(),
                             getValue(I.getOperand(1))));
    return 0;
  case Intrinsic::ctlz_i8:
  case Intrinsic::ctlz_i16:
  case Intrinsic::ctlz_i32:
  case Intrinsic::ctlz_i64:
    setValue(&I, DAG.getNode(ISD::CTLZ,
                             getValue(I.getOperand(1)).getValueType(),
                             getValue(I.getOperand(1))));
    return 0;
  case Intrinsic::ctpop_i8:
  case Intrinsic::ctpop_i16:
  case Intrinsic::ctpop_i32:
  case Intrinsic::ctpop_i64:
    setValue(&I, DAG.getNode(ISD::CTPOP,
                             getValue(I.getOperand(1)).getValueType(),
                             getValue(I.getOperand(1))));
    return 0;
  case Intrinsic::stacksave: {
    SDOperand Op = getRoot();
    SDOperand Tmp = DAG.getNode(ISD::STACKSAVE,
              DAG.getNodeValueTypes(TLI.getPointerTy(), MVT::Other), 2, &Op, 1);
    setValue(&I, Tmp);
    DAG.setRoot(Tmp.getValue(1));
    return 0;
  }
  case Intrinsic::stackrestore: {
    SDOperand Tmp = getValue(I.getOperand(1));
    DAG.setRoot(DAG.getNode(ISD::STACKRESTORE, MVT::Other, getRoot(), Tmp));
    return 0;
  }
  case Intrinsic::prefetch:
    // FIXME: Currently discarding prefetches.
    return 0;
  }
}


void SelectionDAGLowering::visitCall(CallInst &I) {
  const char *RenameFn = 0;
  if (Function *F = I.getCalledFunction()) {
    if (F->isExternal())
      if (unsigned IID = F->getIntrinsicID()) {
        RenameFn = visitIntrinsicCall(I, IID);
        if (!RenameFn)
          return;
      } else {    // Not an LLVM intrinsic.
        const std::string &Name = F->getName();
        if (Name[0] == 'c' && (Name == "copysign" || Name == "copysignf")) {
          if (I.getNumOperands() == 3 &&   // Basic sanity checks.
              I.getOperand(1)->getType()->isFloatingPoint() &&
              I.getType() == I.getOperand(1)->getType() &&
              I.getType() == I.getOperand(2)->getType()) {
            SDOperand LHS = getValue(I.getOperand(1));
            SDOperand RHS = getValue(I.getOperand(2));
            setValue(&I, DAG.getNode(ISD::FCOPYSIGN, LHS.getValueType(),
                                     LHS, RHS));
            return;
          }
        } else if (Name[0] == 'f' && (Name == "fabs" || Name == "fabsf")) {
          if (I.getNumOperands() == 2 &&   // Basic sanity checks.
              I.getOperand(1)->getType()->isFloatingPoint() &&
              I.getType() == I.getOperand(1)->getType()) {
            SDOperand Tmp = getValue(I.getOperand(1));
            setValue(&I, DAG.getNode(ISD::FABS, Tmp.getValueType(), Tmp));
            return;
          }
        } else if (Name[0] == 's' && (Name == "sin" || Name == "sinf")) {
          if (I.getNumOperands() == 2 &&   // Basic sanity checks.
              I.getOperand(1)->getType()->isFloatingPoint() &&
              I.getType() == I.getOperand(1)->getType()) {
            SDOperand Tmp = getValue(I.getOperand(1));
            setValue(&I, DAG.getNode(ISD::FSIN, Tmp.getValueType(), Tmp));
            return;
          }
        } else if (Name[0] == 'c' && (Name == "cos" || Name == "cosf")) {
          if (I.getNumOperands() == 2 &&   // Basic sanity checks.
              I.getOperand(1)->getType()->isFloatingPoint() &&
              I.getType() == I.getOperand(1)->getType()) {
            SDOperand Tmp = getValue(I.getOperand(1));
            setValue(&I, DAG.getNode(ISD::FCOS, Tmp.getValueType(), Tmp));
            return;
          }
        }
      }
  } else if (isa<InlineAsm>(I.getOperand(0))) {
    visitInlineAsm(I);
    return;
  }

  SDOperand Callee;
  if (!RenameFn)
    Callee = getValue(I.getOperand(0));
  else
    Callee = DAG.getExternalSymbol(RenameFn, TLI.getPointerTy());
  std::vector<std::pair<SDOperand, const Type*> > Args;
  Args.reserve(I.getNumOperands());
  for (unsigned i = 1, e = I.getNumOperands(); i != e; ++i) {
    Value *Arg = I.getOperand(i);
    SDOperand ArgNode = getValue(Arg);
    Args.push_back(std::make_pair(ArgNode, Arg->getType()));
  }

  const PointerType *PT = cast<PointerType>(I.getCalledValue()->getType());
  const FunctionType *FTy = cast<FunctionType>(PT->getElementType());

  std::pair<SDOperand,SDOperand> Result =
    TLI.LowerCallTo(getRoot(), I.getType(), FTy->isVarArg(), I.getCallingConv(),
                    I.isTailCall(), Callee, Args, DAG);
  if (I.getType() != Type::VoidTy)
    setValue(&I, Result.first);
  DAG.setRoot(Result.second);
}

SDOperand RegsForValue::getCopyFromRegs(SelectionDAG &DAG,
                                        SDOperand &Chain, SDOperand &Flag)const{
  SDOperand Val = DAG.getCopyFromReg(Chain, Regs[0], RegVT, Flag);
  Chain = Val.getValue(1);
  Flag  = Val.getValue(2);
  
  // If the result was expanded, copy from the top part.
  if (Regs.size() > 1) {
    assert(Regs.size() == 2 &&
           "Cannot expand to more than 2 elts yet!");
    SDOperand Hi = DAG.getCopyFromReg(Chain, Regs[1], RegVT, Flag);
    Chain = Hi.getValue(1);
    Flag  = Hi.getValue(2);
    if (DAG.getTargetLoweringInfo().isLittleEndian())
      return DAG.getNode(ISD::BUILD_PAIR, ValueVT, Val, Hi);
    else
      return DAG.getNode(ISD::BUILD_PAIR, ValueVT, Hi, Val);
  }

  // Otherwise, if the return value was promoted or extended, truncate it to the
  // appropriate type.
  if (RegVT == ValueVT)
    return Val;
  
  if (MVT::isInteger(RegVT)) {
    if (ValueVT < RegVT)
      return DAG.getNode(ISD::TRUNCATE, ValueVT, Val);
    else
      return DAG.getNode(ISD::ANY_EXTEND, ValueVT, Val);
  } else {
    return DAG.getNode(ISD::FP_ROUND, ValueVT, Val);
  }
}

/// getCopyToRegs - Emit a series of CopyToReg nodes that copies the
/// specified value into the registers specified by this object.  This uses 
/// Chain/Flag as the input and updates them for the output Chain/Flag.
void RegsForValue::getCopyToRegs(SDOperand Val, SelectionDAG &DAG,
                                 SDOperand &Chain, SDOperand &Flag,
                                 MVT::ValueType PtrVT) const {
  if (Regs.size() == 1) {
    // If there is a single register and the types differ, this must be
    // a promotion.
    if (RegVT != ValueVT) {
      if (MVT::isInteger(RegVT)) {
        if (RegVT < ValueVT)
          Val = DAG.getNode(ISD::TRUNCATE, RegVT, Val);
        else
          Val = DAG.getNode(ISD::ANY_EXTEND, RegVT, Val);
      } else
        Val = DAG.getNode(ISD::FP_EXTEND, RegVT, Val);
    }
    Chain = DAG.getCopyToReg(Chain, Regs[0], Val, Flag);
    Flag = Chain.getValue(1);
  } else {
    std::vector<unsigned> R(Regs);
    if (!DAG.getTargetLoweringInfo().isLittleEndian())
      std::reverse(R.begin(), R.end());
    
    for (unsigned i = 0, e = R.size(); i != e; ++i) {
      SDOperand Part = DAG.getNode(ISD::EXTRACT_ELEMENT, RegVT, Val, 
                                   DAG.getConstant(i, PtrVT));
      Chain = DAG.getCopyToReg(Chain, R[i], Part, Flag);
      Flag = Chain.getValue(1);
    }
  }
}

/// AddInlineAsmOperands - Add this value to the specified inlineasm node
/// operand list.  This adds the code marker and includes the number of 
/// values added into it.
void RegsForValue::AddInlineAsmOperands(unsigned Code, SelectionDAG &DAG,
                                        std::vector<SDOperand> &Ops) const {
  Ops.push_back(DAG.getConstant(Code | (Regs.size() << 3), MVT::i32));
  for (unsigned i = 0, e = Regs.size(); i != e; ++i)
    Ops.push_back(DAG.getRegister(Regs[i], RegVT));
}

/// isAllocatableRegister - If the specified register is safe to allocate, 
/// i.e. it isn't a stack pointer or some other special register, return the
/// register class for the register.  Otherwise, return null.
static const TargetRegisterClass *
isAllocatableRegister(unsigned Reg, MachineFunction &MF,
                      const TargetLowering &TLI, const MRegisterInfo *MRI) {
  MVT::ValueType FoundVT = MVT::Other;
  const TargetRegisterClass *FoundRC = 0;
  for (MRegisterInfo::regclass_iterator RCI = MRI->regclass_begin(),
       E = MRI->regclass_end(); RCI != E; ++RCI) {
    MVT::ValueType ThisVT = MVT::Other;

    const TargetRegisterClass *RC = *RCI;
    // If none of the the value types for this register class are valid, we 
    // can't use it.  For example, 64-bit reg classes on 32-bit targets.
    for (TargetRegisterClass::vt_iterator I = RC->vt_begin(), E = RC->vt_end();
         I != E; ++I) {
      if (TLI.isTypeLegal(*I)) {
        // If we have already found this register in a different register class,
        // choose the one with the largest VT specified.  For example, on
        // PowerPC, we favor f64 register classes over f32.
        if (FoundVT == MVT::Other || 
            MVT::getSizeInBits(FoundVT) < MVT::getSizeInBits(*I)) {
          ThisVT = *I;
          break;
        }
      }
    }
    
    if (ThisVT == MVT::Other) continue;
    
    // NOTE: This isn't ideal.  In particular, this might allocate the
    // frame pointer in functions that need it (due to them not being taken
    // out of allocation, because a variable sized allocation hasn't been seen
    // yet).  This is a slight code pessimization, but should still work.
    for (TargetRegisterClass::iterator I = RC->allocation_order_begin(MF),
         E = RC->allocation_order_end(MF); I != E; ++I)
      if (*I == Reg) {
        // We found a matching register class.  Keep looking at others in case
        // we find one with larger registers that this physreg is also in.
        FoundRC = RC;
        FoundVT = ThisVT;
        break;
      }
  }
  return FoundRC;
}    

RegsForValue SelectionDAGLowering::
GetRegistersForValue(const std::string &ConstrCode,
                     MVT::ValueType VT, bool isOutReg, bool isInReg,
                     std::set<unsigned> &OutputRegs, 
                     std::set<unsigned> &InputRegs) {
  std::pair<unsigned, const TargetRegisterClass*> PhysReg = 
    TLI.getRegForInlineAsmConstraint(ConstrCode, VT);
  std::vector<unsigned> Regs;

  unsigned NumRegs = VT != MVT::Other ? TLI.getNumElements(VT) : 1;
  MVT::ValueType RegVT;
  MVT::ValueType ValueVT = VT;
  
  // If this is a constraint for a specific physical register, like {r17},
  // assign it now.
  if (PhysReg.first) {
    if (VT == MVT::Other)
      ValueVT = *PhysReg.second->vt_begin();
    
    // Get the actual register value type.  This is important, because the user
    // may have asked for (e.g.) the AX register in i32 type.  We need to
    // remember that AX is actually i16 to get the right extension.
    RegVT = *PhysReg.second->vt_begin();
    
    // This is a explicit reference to a physical register.
    Regs.push_back(PhysReg.first);

    // If this is an expanded reference, add the rest of the regs to Regs.
    if (NumRegs != 1) {
      TargetRegisterClass::iterator I = PhysReg.second->begin();
      TargetRegisterClass::iterator E = PhysReg.second->end();
      for (; *I != PhysReg.first; ++I)
        assert(I != E && "Didn't find reg!"); 
      
      // Already added the first reg.
      --NumRegs; ++I;
      for (; NumRegs; --NumRegs, ++I) {
        assert(I != E && "Ran out of registers to allocate!");
        Regs.push_back(*I);
      }
    }
    return RegsForValue(Regs, RegVT, ValueVT);
  }
  
  // Otherwise, if this was a reference to an LLVM register class, create vregs
  // for this reference.
  std::vector<unsigned> RegClassRegs;
  if (PhysReg.second) {
    // If this is an early clobber or tied register, our regalloc doesn't know
    // how to maintain the constraint.  If it isn't, go ahead and create vreg
    // and let the regalloc do the right thing.
    if (!isOutReg || !isInReg) {
      if (VT == MVT::Other)
        ValueVT = *PhysReg.second->vt_begin();
      RegVT = *PhysReg.second->vt_begin();

      // Create the appropriate number of virtual registers.
      SSARegMap *RegMap = DAG.getMachineFunction().getSSARegMap();
      for (; NumRegs; --NumRegs)
        Regs.push_back(RegMap->createVirtualRegister(PhysReg.second));
      
      return RegsForValue(Regs, RegVT, ValueVT);
    }
    
    // Otherwise, we can't allocate it.  Let the code below figure out how to
    // maintain these constraints.
    RegClassRegs.assign(PhysReg.second->begin(), PhysReg.second->end());
    
  } else {
    // This is a reference to a register class that doesn't directly correspond
    // to an LLVM register class.  Allocate NumRegs consecutive, available,
    // registers from the class.
    RegClassRegs = TLI.getRegClassForInlineAsmConstraint(ConstrCode, VT);
  }

  const MRegisterInfo *MRI = DAG.getTarget().getRegisterInfo();
  MachineFunction &MF = *CurMBB->getParent();
  unsigned NumAllocated = 0;
  for (unsigned i = 0, e = RegClassRegs.size(); i != e; ++i) {
    unsigned Reg = RegClassRegs[i];
    // See if this register is available.
    if ((isOutReg && OutputRegs.count(Reg)) ||   // Already used.
        (isInReg  && InputRegs.count(Reg))) {    // Already used.
      // Make sure we find consecutive registers.
      NumAllocated = 0;
      continue;
    }
    
    // Check to see if this register is allocatable (i.e. don't give out the
    // stack pointer).
    const TargetRegisterClass *RC = isAllocatableRegister(Reg, MF, TLI, MRI);
    if (!RC) {
      // Make sure we find consecutive registers.
      NumAllocated = 0;
      continue;
    }
    
    // Okay, this register is good, we can use it.
    ++NumAllocated;

    // If we allocated enough consecutive   
    if (NumAllocated == NumRegs) {
      unsigned RegStart = (i-NumAllocated)+1;
      unsigned RegEnd   = i+1;
      // Mark all of the allocated registers used.
      for (unsigned i = RegStart; i != RegEnd; ++i) {
        unsigned Reg = RegClassRegs[i];
        Regs.push_back(Reg);
        if (isOutReg) OutputRegs.insert(Reg);    // Mark reg used.
        if (isInReg)  InputRegs.insert(Reg);     // Mark reg used.
      }
      
      return RegsForValue(Regs, *RC->vt_begin(), VT);
    }
  }
  
  // Otherwise, we couldn't allocate enough registers for this.
  return RegsForValue();
}


/// visitInlineAsm - Handle a call to an InlineAsm object.
///
void SelectionDAGLowering::visitInlineAsm(CallInst &I) {
  InlineAsm *IA = cast<InlineAsm>(I.getOperand(0));
  
  SDOperand AsmStr = DAG.getTargetExternalSymbol(IA->getAsmString().c_str(),
                                                 MVT::Other);

  std::vector<InlineAsm::ConstraintInfo> Constraints = IA->ParseConstraints();
  std::vector<MVT::ValueType> ConstraintVTs;
  
  /// AsmNodeOperands - A list of pairs.  The first element is a register, the
  /// second is a bitfield where bit #0 is set if it is a use and bit #1 is set
  /// if it is a def of that register.
  std::vector<SDOperand> AsmNodeOperands;
  AsmNodeOperands.push_back(SDOperand());  // reserve space for input chain
  AsmNodeOperands.push_back(AsmStr);
  
  SDOperand Chain = getRoot();
  SDOperand Flag;
  
  // We fully assign registers here at isel time.  This is not optimal, but
  // should work.  For register classes that correspond to LLVM classes, we
  // could let the LLVM RA do its thing, but we currently don't.  Do a prepass
  // over the constraints, collecting fixed registers that we know we can't use.
  std::set<unsigned> OutputRegs, InputRegs;
  unsigned OpNum = 1;
  for (unsigned i = 0, e = Constraints.size(); i != e; ++i) {
    assert(Constraints[i].Codes.size() == 1 && "Only handles one code so far!");
    std::string &ConstraintCode = Constraints[i].Codes[0];
    
    MVT::ValueType OpVT;

    // Compute the value type for each operand and add it to ConstraintVTs.
    switch (Constraints[i].Type) {
    case InlineAsm::isOutput:
      if (!Constraints[i].isIndirectOutput) {
        assert(I.getType() != Type::VoidTy && "Bad inline asm!");
        OpVT = TLI.getValueType(I.getType());
      } else {
        const Type *OpTy = I.getOperand(OpNum)->getType();
        OpVT = TLI.getValueType(cast<PointerType>(OpTy)->getElementType());
        OpNum++;  // Consumes a call operand.
      }
      break;
    case InlineAsm::isInput:
      OpVT = TLI.getValueType(I.getOperand(OpNum)->getType());
      OpNum++;  // Consumes a call operand.
      break;
    case InlineAsm::isClobber:
      OpVT = MVT::Other;
      break;
    }
    
    ConstraintVTs.push_back(OpVT);

    if (TLI.getRegForInlineAsmConstraint(ConstraintCode, OpVT).first == 0)
      continue;  // Not assigned a fixed reg.
    
    // Build a list of regs that this operand uses.  This always has a single
    // element for promoted/expanded operands.
    RegsForValue Regs = GetRegistersForValue(ConstraintCode, OpVT,
                                             false, false,
                                             OutputRegs, InputRegs);
    
    switch (Constraints[i].Type) {
    case InlineAsm::isOutput:
      // We can't assign any other output to this register.
      OutputRegs.insert(Regs.Regs.begin(), Regs.Regs.end());
      // If this is an early-clobber output, it cannot be assigned to the same
      // value as the input reg.
      if (Constraints[i].isEarlyClobber || Constraints[i].hasMatchingInput)
        InputRegs.insert(Regs.Regs.begin(), Regs.Regs.end());
      break;
    case InlineAsm::isInput:
      // We can't assign any other input to this register.
      InputRegs.insert(Regs.Regs.begin(), Regs.Regs.end());
      break;
    case InlineAsm::isClobber:
      // Clobbered regs cannot be used as inputs or outputs.
      InputRegs.insert(Regs.Regs.begin(), Regs.Regs.end());
      OutputRegs.insert(Regs.Regs.begin(), Regs.Regs.end());
      break;
    }
  }      
  
  // Loop over all of the inputs, copying the operand values into the
  // appropriate registers and processing the output regs.
  RegsForValue RetValRegs;
  std::vector<std::pair<RegsForValue, Value*> > IndirectStoresToEmit;
  OpNum = 1;
  
  for (unsigned i = 0, e = Constraints.size(); i != e; ++i) {
    assert(Constraints[i].Codes.size() == 1 && "Only handles one code so far!");
    std::string &ConstraintCode = Constraints[i].Codes[0];

    switch (Constraints[i].Type) {
    case InlineAsm::isOutput: {
      TargetLowering::ConstraintType CTy = TargetLowering::C_RegisterClass;
      if (ConstraintCode.size() == 1)   // not a physreg name.
        CTy = TLI.getConstraintType(ConstraintCode[0]);
      
      if (CTy == TargetLowering::C_Memory) {
        // Memory output.
        SDOperand InOperandVal = getValue(I.getOperand(OpNum));
        
        // Check that the operand (the address to store to) isn't a float.
        if (!MVT::isInteger(InOperandVal.getValueType()))
          assert(0 && "MATCH FAIL!");
        
        if (!Constraints[i].isIndirectOutput)
          assert(0 && "MATCH FAIL!");

        OpNum++;  // Consumes a call operand.
        
        // Extend/truncate to the right pointer type if needed.
        MVT::ValueType PtrType = TLI.getPointerTy();
        if (InOperandVal.getValueType() < PtrType)
          InOperandVal = DAG.getNode(ISD::ZERO_EXTEND, PtrType, InOperandVal);
        else if (InOperandVal.getValueType() > PtrType)
          InOperandVal = DAG.getNode(ISD::TRUNCATE, PtrType, InOperandVal);
        
        // Add information to the INLINEASM node to know about this output.
        unsigned ResOpType = 4/*MEM*/ | (1 << 3);
        AsmNodeOperands.push_back(DAG.getConstant(ResOpType, MVT::i32));
        AsmNodeOperands.push_back(InOperandVal);
        break;
      }

      // Otherwise, this is a register output.
      assert(CTy == TargetLowering::C_RegisterClass && "Unknown op type!");

      // If this is an early-clobber output, or if there is an input
      // constraint that matches this, we need to reserve the input register
      // so no other inputs allocate to it.
      bool UsesInputRegister = false;
      if (Constraints[i].isEarlyClobber || Constraints[i].hasMatchingInput)
        UsesInputRegister = true;
      
      // Copy the output from the appropriate register.  Find a register that
      // we can use.
      RegsForValue Regs =
        GetRegistersForValue(ConstraintCode, ConstraintVTs[i],
                             true, UsesInputRegister, 
                             OutputRegs, InputRegs);
      if (Regs.Regs.empty()) {
        cerr << "Couldn't allocate output reg for contraint '"
             << ConstraintCode << "'!\n";
        exit(1);
      }

      if (!Constraints[i].isIndirectOutput) {
        assert(RetValRegs.Regs.empty() &&
               "Cannot have multiple output constraints yet!");
        assert(I.getType() != Type::VoidTy && "Bad inline asm!");
        RetValRegs = Regs;
      } else {
        IndirectStoresToEmit.push_back(std::make_pair(Regs, 
                                                      I.getOperand(OpNum)));
        OpNum++;  // Consumes a call operand.
      }
      
      // Add information to the INLINEASM node to know that this register is
      // set.
      Regs.AddInlineAsmOperands(2 /*REGDEF*/, DAG, AsmNodeOperands);
      break;
    }
    case InlineAsm::isInput: {
      SDOperand InOperandVal = getValue(I.getOperand(OpNum));
      OpNum++;  // Consumes a call operand.
      
      if (isdigit(ConstraintCode[0])) {    // Matching constraint?
        // If this is required to match an output register we have already set,
        // just use its register.
        unsigned OperandNo = atoi(ConstraintCode.c_str());
        
        // Scan until we find the definition we already emitted of this operand.
        // When we find it, create a RegsForValue operand.
        unsigned CurOp = 2;  // The first operand.
        for (; OperandNo; --OperandNo) {
          // Advance to the next operand.
          unsigned NumOps = 
            cast<ConstantSDNode>(AsmNodeOperands[CurOp])->getValue();
          assert(((NumOps & 7) == 2 /*REGDEF*/ ||
                  (NumOps & 7) == 4 /*MEM*/) &&
                 "Skipped past definitions?");
          CurOp += (NumOps>>3)+1;
        }

        unsigned NumOps = 
          cast<ConstantSDNode>(AsmNodeOperands[CurOp])->getValue();
        assert((NumOps & 7) == 2 /*REGDEF*/ &&
               "Skipped past definitions?");
        
        // Add NumOps>>3 registers to MatchedRegs.
        RegsForValue MatchedRegs;
        MatchedRegs.ValueVT = InOperandVal.getValueType();
        MatchedRegs.RegVT   = AsmNodeOperands[CurOp+1].getValueType();
        for (unsigned i = 0, e = NumOps>>3; i != e; ++i) {
          unsigned Reg=cast<RegisterSDNode>(AsmNodeOperands[++CurOp])->getReg();
          MatchedRegs.Regs.push_back(Reg);
        }
        
        // Use the produced MatchedRegs object to 
        MatchedRegs.getCopyToRegs(InOperandVal, DAG, Chain, Flag,
                                  TLI.getPointerTy());
        MatchedRegs.AddInlineAsmOperands(1 /*REGUSE*/, DAG, AsmNodeOperands);
        break;
      }
      
      TargetLowering::ConstraintType CTy = TargetLowering::C_RegisterClass;
      if (ConstraintCode.size() == 1)   // not a physreg name.
        CTy = TLI.getConstraintType(ConstraintCode[0]);
        
      if (CTy == TargetLowering::C_Other) {
        InOperandVal = TLI.isOperandValidForConstraint(InOperandVal,
                                                       ConstraintCode[0], DAG);
        if (!InOperandVal.Val) {
          cerr << "Invalid operand for inline asm constraint '"
               << ConstraintCode << "'!\n";
          exit(1);
        }
        
        // Add information to the INLINEASM node to know about this input.
        unsigned ResOpType = 3 /*IMM*/ | (1 << 3);
        AsmNodeOperands.push_back(DAG.getConstant(ResOpType, MVT::i32));
        AsmNodeOperands.push_back(InOperandVal);
        break;
      } else if (CTy == TargetLowering::C_Memory) {
        // Memory input.
        
        // Check that the operand isn't a float.
        if (!MVT::isInteger(InOperandVal.getValueType()))
          assert(0 && "MATCH FAIL!");
        
        // Extend/truncate to the right pointer type if needed.
        MVT::ValueType PtrType = TLI.getPointerTy();
        if (InOperandVal.getValueType() < PtrType)
          InOperandVal = DAG.getNode(ISD::ZERO_EXTEND, PtrType, InOperandVal);
        else if (InOperandVal.getValueType() > PtrType)
          InOperandVal = DAG.getNode(ISD::TRUNCATE, PtrType, InOperandVal);

        // Add information to the INLINEASM node to know about this input.
        unsigned ResOpType = 4/*MEM*/ | (1 << 3);
        AsmNodeOperands.push_back(DAG.getConstant(ResOpType, MVT::i32));
        AsmNodeOperands.push_back(InOperandVal);
        break;
      }
        
      assert(CTy == TargetLowering::C_RegisterClass && "Unknown op type!");

      // Copy the input into the appropriate registers.
      RegsForValue InRegs =
        GetRegistersForValue(ConstraintCode, ConstraintVTs[i],
                             false, true, OutputRegs, InputRegs);
      // FIXME: should be match fail.
      assert(!InRegs.Regs.empty() && "Couldn't allocate input reg!");

      InRegs.getCopyToRegs(InOperandVal, DAG, Chain, Flag, TLI.getPointerTy());
      
      InRegs.AddInlineAsmOperands(1/*REGUSE*/, DAG, AsmNodeOperands);
      break;
    }
    case InlineAsm::isClobber: {
      RegsForValue ClobberedRegs =
        GetRegistersForValue(ConstraintCode, MVT::Other, false, false,
                             OutputRegs, InputRegs);
      // Add the clobbered value to the operand list, so that the register
      // allocator is aware that the physreg got clobbered.
      if (!ClobberedRegs.Regs.empty())
        ClobberedRegs.AddInlineAsmOperands(2/*REGDEF*/, DAG, AsmNodeOperands);
      break;
    }
    }
  }
  
  // Finish up input operands.
  AsmNodeOperands[0] = Chain;
  if (Flag.Val) AsmNodeOperands.push_back(Flag);
  
  Chain = DAG.getNode(ISD::INLINEASM, 
                      DAG.getNodeValueTypes(MVT::Other, MVT::Flag), 2,
                      &AsmNodeOperands[0], AsmNodeOperands.size());
  Flag = Chain.getValue(1);

  // If this asm returns a register value, copy the result from that register
  // and set it as the value of the call.
  if (!RetValRegs.Regs.empty())
    setValue(&I, RetValRegs.getCopyFromRegs(DAG, Chain, Flag));
  
  std::vector<std::pair<SDOperand, Value*> > StoresToEmit;
  
  // Process indirect outputs, first output all of the flagged copies out of
  // physregs.
  for (unsigned i = 0, e = IndirectStoresToEmit.size(); i != e; ++i) {
    RegsForValue &OutRegs = IndirectStoresToEmit[i].first;
    Value *Ptr = IndirectStoresToEmit[i].second;
    SDOperand OutVal = OutRegs.getCopyFromRegs(DAG, Chain, Flag);
    StoresToEmit.push_back(std::make_pair(OutVal, Ptr));
  }
  
  // Emit the non-flagged stores from the physregs.
  SmallVector<SDOperand, 8> OutChains;
  for (unsigned i = 0, e = StoresToEmit.size(); i != e; ++i)
    OutChains.push_back(DAG.getStore(Chain,  StoresToEmit[i].first,
                                    getValue(StoresToEmit[i].second),
                                    StoresToEmit[i].second, 0));
  if (!OutChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, MVT::Other,
                        &OutChains[0], OutChains.size());
  DAG.setRoot(Chain);
}


void SelectionDAGLowering::visitMalloc(MallocInst &I) {
  SDOperand Src = getValue(I.getOperand(0));

  MVT::ValueType IntPtr = TLI.getPointerTy();

  if (IntPtr < Src.getValueType())
    Src = DAG.getNode(ISD::TRUNCATE, IntPtr, Src);
  else if (IntPtr > Src.getValueType())
    Src = DAG.getNode(ISD::ZERO_EXTEND, IntPtr, Src);

  // Scale the source by the type size.
  uint64_t ElementSize = TD->getTypeSize(I.getType()->getElementType());
  Src = DAG.getNode(ISD::MUL, Src.getValueType(),
                    Src, getIntPtrConstant(ElementSize));

  std::vector<std::pair<SDOperand, const Type*> > Args;
  Args.push_back(std::make_pair(Src, TLI.getTargetData()->getIntPtrType()));

  std::pair<SDOperand,SDOperand> Result =
    TLI.LowerCallTo(getRoot(), I.getType(), false, CallingConv::C, true,
                    DAG.getExternalSymbol("malloc", IntPtr),
                    Args, DAG);
  setValue(&I, Result.first);  // Pointers always fit in registers
  DAG.setRoot(Result.second);
}

void SelectionDAGLowering::visitFree(FreeInst &I) {
  std::vector<std::pair<SDOperand, const Type*> > Args;
  Args.push_back(std::make_pair(getValue(I.getOperand(0)),
                                TLI.getTargetData()->getIntPtrType()));
  MVT::ValueType IntPtr = TLI.getPointerTy();
  std::pair<SDOperand,SDOperand> Result =
    TLI.LowerCallTo(getRoot(), Type::VoidTy, false, CallingConv::C, true,
                    DAG.getExternalSymbol("free", IntPtr), Args, DAG);
  DAG.setRoot(Result.second);
}

// InsertAtEndOfBasicBlock - This method should be implemented by targets that
// mark instructions with the 'usesCustomDAGSchedInserter' flag.  These
// instructions are special in various ways, which require special support to
// insert.  The specified MachineInstr is created but not inserted into any
// basic blocks, and the scheduler passes ownership of it to this method.
MachineBasicBlock *TargetLowering::InsertAtEndOfBasicBlock(MachineInstr *MI,
                                                       MachineBasicBlock *MBB) {
  cerr << "If a target marks an instruction with "
       << "'usesCustomDAGSchedInserter', it must implement "
       << "TargetLowering::InsertAtEndOfBasicBlock!\n";
  abort();
  return 0;  
}

void SelectionDAGLowering::visitVAStart(CallInst &I) {
  DAG.setRoot(DAG.getNode(ISD::VASTART, MVT::Other, getRoot(), 
                          getValue(I.getOperand(1)), 
                          DAG.getSrcValue(I.getOperand(1))));
}

void SelectionDAGLowering::visitVAArg(VAArgInst &I) {
  SDOperand V = DAG.getVAArg(TLI.getValueType(I.getType()), getRoot(),
                             getValue(I.getOperand(0)),
                             DAG.getSrcValue(I.getOperand(0)));
  setValue(&I, V);
  DAG.setRoot(V.getValue(1));
}

void SelectionDAGLowering::visitVAEnd(CallInst &I) {
  DAG.setRoot(DAG.getNode(ISD::VAEND, MVT::Other, getRoot(),
                          getValue(I.getOperand(1)), 
                          DAG.getSrcValue(I.getOperand(1))));
}

void SelectionDAGLowering::visitVACopy(CallInst &I) {
  DAG.setRoot(DAG.getNode(ISD::VACOPY, MVT::Other, getRoot(), 
                          getValue(I.getOperand(1)), 
                          getValue(I.getOperand(2)),
                          DAG.getSrcValue(I.getOperand(1)),
                          DAG.getSrcValue(I.getOperand(2))));
}

/// ExpandScalarFormalArgs - Recursively expand the formal_argument node, either
/// bit_convert it or join a pair of them with a BUILD_PAIR when appropriate.
static SDOperand ExpandScalarFormalArgs(MVT::ValueType VT, SDNode *Arg,
                                        unsigned &i, SelectionDAG &DAG,
                                        TargetLowering &TLI) {
  if (TLI.getTypeAction(VT) != TargetLowering::Expand)
    return SDOperand(Arg, i++);

  MVT::ValueType EVT = TLI.getTypeToTransformTo(VT);
  unsigned NumVals = MVT::getSizeInBits(VT) / MVT::getSizeInBits(EVT);
  if (NumVals == 1) {
    return DAG.getNode(ISD::BIT_CONVERT, VT,
                       ExpandScalarFormalArgs(EVT, Arg, i, DAG, TLI));
  } else if (NumVals == 2) {
    SDOperand Lo = ExpandScalarFormalArgs(EVT, Arg, i, DAG, TLI);
    SDOperand Hi = ExpandScalarFormalArgs(EVT, Arg, i, DAG, TLI);
    if (!TLI.isLittleEndian())
      std::swap(Lo, Hi);
    return DAG.getNode(ISD::BUILD_PAIR, VT, Lo, Hi);
  } else {
    // Value scalarized into many values.  Unimp for now.
    assert(0 && "Cannot expand i64 -> i16 yet!");
  }
  return SDOperand();
}

/// TargetLowering::LowerArguments - This is the default LowerArguments
/// implementation, which just inserts a FORMAL_ARGUMENTS node.  FIXME: When all
/// targets are migrated to using FORMAL_ARGUMENTS, this hook should be 
/// integrated into SDISel.
std::vector<SDOperand> 
TargetLowering::LowerArguments(Function &F, SelectionDAG &DAG) {
  // Add CC# and isVararg as operands to the FORMAL_ARGUMENTS node.
  std::vector<SDOperand> Ops;
  Ops.push_back(DAG.getRoot());
  Ops.push_back(DAG.getConstant(F.getCallingConv(), getPointerTy()));
  Ops.push_back(DAG.getConstant(F.isVarArg(), getPointerTy()));

  // Add one result value for each formal argument.
  std::vector<MVT::ValueType> RetVals;
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    MVT::ValueType VT = getValueType(I->getType());
    
    switch (getTypeAction(VT)) {
    default: assert(0 && "Unknown type action!");
    case Legal: 
      RetVals.push_back(VT);
      break;
    case Promote:
      RetVals.push_back(getTypeToTransformTo(VT));
      break;
    case Expand:
      if (VT != MVT::Vector) {
        // If this is a large integer, it needs to be broken up into small
        // integers.  Figure out what the destination type is and how many small
        // integers it turns into.
        MVT::ValueType NVT = getTypeToExpandTo(VT);
        unsigned NumVals = getNumElements(VT);
        for (unsigned i = 0; i != NumVals; ++i)
          RetVals.push_back(NVT);
      } else {
        // Otherwise, this is a vector type.  We only support legal vectors
        // right now.
        unsigned NumElems = cast<PackedType>(I->getType())->getNumElements();
        const Type *EltTy = cast<PackedType>(I->getType())->getElementType();

        // Figure out if there is a Packed type corresponding to this Vector
        // type.  If so, convert to the packed type.
        MVT::ValueType TVT = MVT::getVectorType(getValueType(EltTy), NumElems);
        if (TVT != MVT::Other && isTypeLegal(TVT)) {
          RetVals.push_back(TVT);
        } else {
          assert(0 && "Don't support illegal by-val vector arguments yet!");
        }
      }
      break;
    }
  }

  RetVals.push_back(MVT::Other);
  
  // Create the node.
  SDNode *Result = DAG.getNode(ISD::FORMAL_ARGUMENTS,
                               DAG.getNodeValueTypes(RetVals), RetVals.size(),
                               &Ops[0], Ops.size()).Val;
  
  DAG.setRoot(SDOperand(Result, Result->getNumValues()-1));

  // Set up the return result vector.
  Ops.clear();
  unsigned i = 0;
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    MVT::ValueType VT = getValueType(I->getType());
    
    switch (getTypeAction(VT)) {
    default: assert(0 && "Unknown type action!");
    case Legal: 
      Ops.push_back(SDOperand(Result, i++));
      break;
    case Promote: {
      SDOperand Op(Result, i++);
      if (MVT::isInteger(VT)) {
        unsigned AssertOp = I->getType()->isSigned() ? ISD::AssertSext 
                                                     : ISD::AssertZext;
        Op = DAG.getNode(AssertOp, Op.getValueType(), Op, DAG.getValueType(VT));
        Op = DAG.getNode(ISD::TRUNCATE, VT, Op);
      } else {
        assert(MVT::isFloatingPoint(VT) && "Not int or FP?");
        Op = DAG.getNode(ISD::FP_ROUND, VT, Op);
      }
      Ops.push_back(Op);
      break;
    }
    case Expand:
      if (VT != MVT::Vector) {
        // If this is a large integer or a floating point node that needs to be
        // expanded, it needs to be reassembled from small integers.  Figure out
        // what the source elt type is and how many small integers it is.
        Ops.push_back(ExpandScalarFormalArgs(VT, Result, i, DAG, *this));
      } else {
        // Otherwise, this is a vector type.  We only support legal vectors
        // right now.
        const PackedType *PTy = cast<PackedType>(I->getType());
        unsigned NumElems = PTy->getNumElements();
        const Type *EltTy = PTy->getElementType();

        // Figure out if there is a Packed type corresponding to this Vector
        // type.  If so, convert to the packed type.
        MVT::ValueType TVT = MVT::getVectorType(getValueType(EltTy), NumElems);
        if (TVT != MVT::Other && isTypeLegal(TVT)) {
          SDOperand N = SDOperand(Result, i++);
          // Handle copies from generic vectors to registers.
          N = DAG.getNode(ISD::VBIT_CONVERT, MVT::Vector, N,
                          DAG.getConstant(NumElems, MVT::i32), 
                          DAG.getValueType(getValueType(EltTy)));
          Ops.push_back(N);
        } else {
          assert(0 && "Don't support illegal by-val vector arguments yet!");
          abort();
        }
      }
      break;
    }
  }
  return Ops;
}


/// ExpandScalarCallArgs - Recursively expand call argument node by
/// bit_converting it or extract a pair of elements from the larger  node.
static void ExpandScalarCallArgs(MVT::ValueType VT, SDOperand Arg,
                                 bool isSigned, 
                                 SmallVector<SDOperand, 32> &Ops,
                                 SelectionDAG &DAG,
                                 TargetLowering &TLI) {
  if (TLI.getTypeAction(VT) != TargetLowering::Expand) {
    Ops.push_back(Arg);
    Ops.push_back(DAG.getConstant(isSigned, MVT::i32));
    return;
  }

  MVT::ValueType EVT = TLI.getTypeToTransformTo(VT);
  unsigned NumVals = MVT::getSizeInBits(VT) / MVT::getSizeInBits(EVT);
  if (NumVals == 1) {
    Arg = DAG.getNode(ISD::BIT_CONVERT, EVT, Arg);
    ExpandScalarCallArgs(EVT, Arg, isSigned, Ops, DAG, TLI);
  } else if (NumVals == 2) {
    SDOperand Lo = DAG.getNode(ISD::EXTRACT_ELEMENT, EVT, Arg,
                               DAG.getConstant(0, TLI.getPointerTy()));
    SDOperand Hi = DAG.getNode(ISD::EXTRACT_ELEMENT, EVT, Arg,
                               DAG.getConstant(1, TLI.getPointerTy()));
    if (!TLI.isLittleEndian())
      std::swap(Lo, Hi);
    ExpandScalarCallArgs(EVT, Lo, isSigned, Ops, DAG, TLI);
    ExpandScalarCallArgs(EVT, Hi, isSigned, Ops, DAG, TLI);
  } else {
    // Value scalarized into many values.  Unimp for now.
    assert(0 && "Cannot expand i64 -> i16 yet!");
  }
}

/// TargetLowering::LowerCallTo - This is the default LowerCallTo
/// implementation, which just inserts an ISD::CALL node, which is later custom
/// lowered by the target to something concrete.  FIXME: When all targets are
/// migrated to using ISD::CALL, this hook should be integrated into SDISel.
std::pair<SDOperand, SDOperand>
TargetLowering::LowerCallTo(SDOperand Chain, const Type *RetTy, bool isVarArg,
                            unsigned CallingConv, bool isTailCall, 
                            SDOperand Callee,
                            ArgListTy &Args, SelectionDAG &DAG) {
  SmallVector<SDOperand, 32> Ops;
  Ops.push_back(Chain);   // Op#0 - Chain
  Ops.push_back(DAG.getConstant(CallingConv, getPointerTy())); // Op#1 - CC
  Ops.push_back(DAG.getConstant(isVarArg, getPointerTy()));    // Op#2 - VarArg
  Ops.push_back(DAG.getConstant(isTailCall, getPointerTy()));  // Op#3 - Tail
  Ops.push_back(Callee);
  
  // Handle all of the outgoing arguments.
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    MVT::ValueType VT = getValueType(Args[i].second);
    SDOperand Op = Args[i].first;
    bool isSigned = Args[i].second->isSigned();
    switch (getTypeAction(VT)) {
    default: assert(0 && "Unknown type action!");
    case Legal: 
      Ops.push_back(Op);
      Ops.push_back(DAG.getConstant(isSigned, MVT::i32));
      break;
    case Promote:
      if (MVT::isInteger(VT)) {
        unsigned ExtOp = isSigned ? ISD::SIGN_EXTEND : ISD::ZERO_EXTEND; 
        Op = DAG.getNode(ExtOp, getTypeToTransformTo(VT), Op);
      } else {
        assert(MVT::isFloatingPoint(VT) && "Not int or FP?");
        Op = DAG.getNode(ISD::FP_EXTEND, getTypeToTransformTo(VT), Op);
      }
      Ops.push_back(Op);
      Ops.push_back(DAG.getConstant(isSigned, MVT::i32));
      break;
    case Expand:
      if (VT != MVT::Vector) {
        // If this is a large integer, it needs to be broken down into small
        // integers.  Figure out what the source elt type is and how many small
        // integers it is.
        ExpandScalarCallArgs(VT, Op, isSigned, Ops, DAG, *this);
      } else {
        // Otherwise, this is a vector type.  We only support legal vectors
        // right now.
        const PackedType *PTy = cast<PackedType>(Args[i].second);
        unsigned NumElems = PTy->getNumElements();
        const Type *EltTy = PTy->getElementType();
        
        // Figure out if there is a Packed type corresponding to this Vector
        // type.  If so, convert to the packed type.
        MVT::ValueType TVT = MVT::getVectorType(getValueType(EltTy), NumElems);
        if (TVT != MVT::Other && isTypeLegal(TVT)) {
          // Insert a VBIT_CONVERT of the MVT::Vector type to the packed type.
          Op = DAG.getNode(ISD::VBIT_CONVERT, TVT, Op);
          Ops.push_back(Op);
          Ops.push_back(DAG.getConstant(isSigned, MVT::i32));
        } else {
          assert(0 && "Don't support illegal by-val vector call args yet!");
          abort();
        }
      }
      break;
    }
  }
  
  // Figure out the result value types.
  SmallVector<MVT::ValueType, 4> RetTys;

  if (RetTy != Type::VoidTy) {
    MVT::ValueType VT = getValueType(RetTy);
    switch (getTypeAction(VT)) {
    default: assert(0 && "Unknown type action!");
    case Legal:
      RetTys.push_back(VT);
      break;
    case Promote:
      RetTys.push_back(getTypeToTransformTo(VT));
      break;
    case Expand:
      if (VT != MVT::Vector) {
        // If this is a large integer, it needs to be reassembled from small
        // integers.  Figure out what the source elt type is and how many small
        // integers it is.
        MVT::ValueType NVT = getTypeToExpandTo(VT);
        unsigned NumVals = getNumElements(VT);
        for (unsigned i = 0; i != NumVals; ++i)
          RetTys.push_back(NVT);
      } else {
        // Otherwise, this is a vector type.  We only support legal vectors
        // right now.
        const PackedType *PTy = cast<PackedType>(RetTy);
        unsigned NumElems = PTy->getNumElements();
        const Type *EltTy = PTy->getElementType();
        
        // Figure out if there is a Packed type corresponding to this Vector
        // type.  If so, convert to the packed type.
        MVT::ValueType TVT = MVT::getVectorType(getValueType(EltTy), NumElems);
        if (TVT != MVT::Other && isTypeLegal(TVT)) {
          RetTys.push_back(TVT);
        } else {
          assert(0 && "Don't support illegal by-val vector call results yet!");
          abort();
        }
      }
    }    
  }
  
  RetTys.push_back(MVT::Other);  // Always has a chain.
  
  // Finally, create the CALL node.
  SDOperand Res = DAG.getNode(ISD::CALL,
                              DAG.getVTList(&RetTys[0], RetTys.size()),
                              &Ops[0], Ops.size());
  
  // This returns a pair of operands.  The first element is the
  // return value for the function (if RetTy is not VoidTy).  The second
  // element is the outgoing token chain.
  SDOperand ResVal;
  if (RetTys.size() != 1) {
    MVT::ValueType VT = getValueType(RetTy);
    if (RetTys.size() == 2) {
      ResVal = Res;
      
      // If this value was promoted, truncate it down.
      if (ResVal.getValueType() != VT) {
        if (VT == MVT::Vector) {
          // Insert a VBITCONVERT to convert from the packed result type to the
          // MVT::Vector type.
          unsigned NumElems = cast<PackedType>(RetTy)->getNumElements();
          const Type *EltTy = cast<PackedType>(RetTy)->getElementType();
          
          // Figure out if there is a Packed type corresponding to this Vector
          // type.  If so, convert to the packed type.
          MVT::ValueType TVT = MVT::getVectorType(getValueType(EltTy), NumElems);
          if (TVT != MVT::Other && isTypeLegal(TVT)) {
            // Insert a VBIT_CONVERT of the FORMAL_ARGUMENTS to a
            // "N x PTyElementVT" MVT::Vector type.
            ResVal = DAG.getNode(ISD::VBIT_CONVERT, MVT::Vector, ResVal,
                                 DAG.getConstant(NumElems, MVT::i32), 
                                 DAG.getValueType(getValueType(EltTy)));
          } else {
            abort();
          }
        } else if (MVT::isInteger(VT)) {
          unsigned AssertOp = RetTy->isSigned() ?
                                  ISD::AssertSext : ISD::AssertZext;
          ResVal = DAG.getNode(AssertOp, ResVal.getValueType(), ResVal, 
                               DAG.getValueType(VT));
          ResVal = DAG.getNode(ISD::TRUNCATE, VT, ResVal);
        } else {
          assert(MVT::isFloatingPoint(VT));
          if (getTypeAction(VT) == Expand)
            ResVal = DAG.getNode(ISD::BIT_CONVERT, VT, ResVal);
          else
            ResVal = DAG.getNode(ISD::FP_ROUND, VT, ResVal);
        }
      }
    } else if (RetTys.size() == 3) {
      ResVal = DAG.getNode(ISD::BUILD_PAIR, VT, 
                           Res.getValue(0), Res.getValue(1));
      
    } else {
      assert(0 && "Case not handled yet!");
    }
  }
  
  return std::make_pair(ResVal, Res.getValue(Res.Val->getNumValues()-1));
}



// It is always conservatively correct for llvm.returnaddress and
// llvm.frameaddress to return 0.
//
// FIXME: Change this to insert a FRAMEADDR/RETURNADDR node, and have that be
// expanded to 0 if the target wants.
std::pair<SDOperand, SDOperand>
TargetLowering::LowerFrameReturnAddress(bool isFrameAddr, SDOperand Chain,
                                        unsigned Depth, SelectionDAG &DAG) {
  return std::make_pair(DAG.getConstant(0, getPointerTy()), Chain);
}

SDOperand TargetLowering::LowerOperation(SDOperand Op, SelectionDAG &DAG) {
  assert(0 && "LowerOperation not implemented for this target!");
  abort();
  return SDOperand();
}

SDOperand TargetLowering::CustomPromoteOperation(SDOperand Op,
                                                 SelectionDAG &DAG) {
  assert(0 && "CustomPromoteOperation not implemented for this target!");
  abort();
  return SDOperand();
}

void SelectionDAGLowering::visitFrameReturnAddress(CallInst &I, bool isFrame) {
  unsigned Depth = (unsigned)cast<ConstantInt>(I.getOperand(1))->getZExtValue();
  std::pair<SDOperand,SDOperand> Result =
    TLI.LowerFrameReturnAddress(isFrame, getRoot(), Depth, DAG);
  setValue(&I, Result.first);
  DAG.setRoot(Result.second);
}

/// getMemsetValue - Vectorized representation of the memset value
/// operand.
static SDOperand getMemsetValue(SDOperand Value, MVT::ValueType VT,
                                SelectionDAG &DAG) {
  MVT::ValueType CurVT = VT;
  if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Value)) {
    uint64_t Val   = C->getValue() & 255;
    unsigned Shift = 8;
    while (CurVT != MVT::i8) {
      Val = (Val << Shift) | Val;
      Shift <<= 1;
      CurVT = (MVT::ValueType)((unsigned)CurVT - 1);
    }
    return DAG.getConstant(Val, VT);
  } else {
    Value = DAG.getNode(ISD::ZERO_EXTEND, VT, Value);
    unsigned Shift = 8;
    while (CurVT != MVT::i8) {
      Value =
        DAG.getNode(ISD::OR, VT,
                    DAG.getNode(ISD::SHL, VT, Value,
                                DAG.getConstant(Shift, MVT::i8)), Value);
      Shift <<= 1;
      CurVT = (MVT::ValueType)((unsigned)CurVT - 1);
    }

    return Value;
  }
}

/// getMemsetStringVal - Similar to getMemsetValue. Except this is only
/// used when a memcpy is turned into a memset when the source is a constant
/// string ptr.
static SDOperand getMemsetStringVal(MVT::ValueType VT,
                                    SelectionDAG &DAG, TargetLowering &TLI,
                                    std::string &Str, unsigned Offset) {
  uint64_t Val = 0;
  unsigned MSB = getSizeInBits(VT) / 8;
  if (TLI.isLittleEndian())
    Offset = Offset + MSB - 1;
  for (unsigned i = 0; i != MSB; ++i) {
    Val = (Val << 8) | (unsigned char)Str[Offset];
    Offset += TLI.isLittleEndian() ? -1 : 1;
  }
  return DAG.getConstant(Val, VT);
}

/// getMemBasePlusOffset - Returns base and offset node for the 
static SDOperand getMemBasePlusOffset(SDOperand Base, unsigned Offset,
                                      SelectionDAG &DAG, TargetLowering &TLI) {
  MVT::ValueType VT = Base.getValueType();
  return DAG.getNode(ISD::ADD, VT, Base, DAG.getConstant(Offset, VT));
}

/// MeetsMaxMemopRequirement - Determines if the number of memory ops required
/// to replace the memset / memcpy is below the threshold. It also returns the
/// types of the sequence of  memory ops to perform memset / memcpy.
static bool MeetsMaxMemopRequirement(std::vector<MVT::ValueType> &MemOps,
                                     unsigned Limit, uint64_t Size,
                                     unsigned Align, TargetLowering &TLI) {
  MVT::ValueType VT;

  if (TLI.allowsUnalignedMemoryAccesses()) {
    VT = MVT::i64;
  } else {
    switch (Align & 7) {
    case 0:
      VT = MVT::i64;
      break;
    case 4:
      VT = MVT::i32;
      break;
    case 2:
      VT = MVT::i16;
      break;
    default:
      VT = MVT::i8;
      break;
    }
  }

  MVT::ValueType LVT = MVT::i64;
  while (!TLI.isTypeLegal(LVT))
    LVT = (MVT::ValueType)((unsigned)LVT - 1);
  assert(MVT::isInteger(LVT));

  if (VT > LVT)
    VT = LVT;

  unsigned NumMemOps = 0;
  while (Size != 0) {
    unsigned VTSize = getSizeInBits(VT) / 8;
    while (VTSize > Size) {
      VT = (MVT::ValueType)((unsigned)VT - 1);
      VTSize >>= 1;
    }
    assert(MVT::isInteger(VT));

    if (++NumMemOps > Limit)
      return false;
    MemOps.push_back(VT);
    Size -= VTSize;
  }

  return true;
}

void SelectionDAGLowering::visitMemIntrinsic(CallInst &I, unsigned Op) {
  SDOperand Op1 = getValue(I.getOperand(1));
  SDOperand Op2 = getValue(I.getOperand(2));
  SDOperand Op3 = getValue(I.getOperand(3));
  SDOperand Op4 = getValue(I.getOperand(4));
  unsigned Align = (unsigned)cast<ConstantSDNode>(Op4)->getValue();
  if (Align == 0) Align = 1;

  if (ConstantSDNode *Size = dyn_cast<ConstantSDNode>(Op3)) {
    std::vector<MVT::ValueType> MemOps;

    // Expand memset / memcpy to a series of load / store ops
    // if the size operand falls below a certain threshold.
    SmallVector<SDOperand, 8> OutChains;
    switch (Op) {
    default: break;  // Do nothing for now.
    case ISD::MEMSET: {
      if (MeetsMaxMemopRequirement(MemOps, TLI.getMaxStoresPerMemset(),
                                   Size->getValue(), Align, TLI)) {
        unsigned NumMemOps = MemOps.size();
        unsigned Offset = 0;
        for (unsigned i = 0; i < NumMemOps; i++) {
          MVT::ValueType VT = MemOps[i];
          unsigned VTSize = getSizeInBits(VT) / 8;
          SDOperand Value = getMemsetValue(Op2, VT, DAG);
          SDOperand Store = DAG.getStore(getRoot(), Value,
                                    getMemBasePlusOffset(Op1, Offset, DAG, TLI),
                                         I.getOperand(1), Offset);
          OutChains.push_back(Store);
          Offset += VTSize;
        }
      }
      break;
    }
    case ISD::MEMCPY: {
      if (MeetsMaxMemopRequirement(MemOps, TLI.getMaxStoresPerMemcpy(),
                                   Size->getValue(), Align, TLI)) {
        unsigned NumMemOps = MemOps.size();
        unsigned SrcOff = 0, DstOff = 0, SrcDelta = 0;
        GlobalAddressSDNode *G = NULL;
        std::string Str;
        bool CopyFromStr = false;

        if (Op2.getOpcode() == ISD::GlobalAddress)
          G = cast<GlobalAddressSDNode>(Op2);
        else if (Op2.getOpcode() == ISD::ADD &&
                 Op2.getOperand(0).getOpcode() == ISD::GlobalAddress &&
                 Op2.getOperand(1).getOpcode() == ISD::Constant) {
          G = cast<GlobalAddressSDNode>(Op2.getOperand(0));
          SrcDelta = cast<ConstantSDNode>(Op2.getOperand(1))->getValue();
        }
        if (G) {
          GlobalVariable *GV = dyn_cast<GlobalVariable>(G->getGlobal());
          if (GV && GV->isConstant()) {
            Str = GV->getStringValue(false);
            if (!Str.empty()) {
              CopyFromStr = true;
              SrcOff += SrcDelta;
            }
          }
        }

        for (unsigned i = 0; i < NumMemOps; i++) {
          MVT::ValueType VT = MemOps[i];
          unsigned VTSize = getSizeInBits(VT) / 8;
          SDOperand Value, Chain, Store;

          if (CopyFromStr) {
            Value = getMemsetStringVal(VT, DAG, TLI, Str, SrcOff);
            Chain = getRoot();
            Store =
              DAG.getStore(Chain, Value,
                           getMemBasePlusOffset(Op1, DstOff, DAG, TLI),
                           I.getOperand(1), DstOff);
          } else {
            Value = DAG.getLoad(VT, getRoot(),
                        getMemBasePlusOffset(Op2, SrcOff, DAG, TLI),
                        I.getOperand(2), SrcOff);
            Chain = Value.getValue(1);
            Store =
              DAG.getStore(Chain, Value,
                           getMemBasePlusOffset(Op1, DstOff, DAG, TLI),
                           I.getOperand(1), DstOff);
          }
          OutChains.push_back(Store);
          SrcOff += VTSize;
          DstOff += VTSize;
        }
      }
      break;
    }
    }

    if (!OutChains.empty()) {
      DAG.setRoot(DAG.getNode(ISD::TokenFactor, MVT::Other,
                  &OutChains[0], OutChains.size()));
      return;
    }
  }

  DAG.setRoot(DAG.getNode(Op, MVT::Other, getRoot(), Op1, Op2, Op3, Op4));
}

//===----------------------------------------------------------------------===//
// SelectionDAGISel code
//===----------------------------------------------------------------------===//

unsigned SelectionDAGISel::MakeReg(MVT::ValueType VT) {
  return RegMap->createVirtualRegister(TLI.getRegClassFor(VT));
}

void SelectionDAGISel::getAnalysisUsage(AnalysisUsage &AU) const {
  // FIXME: we only modify the CFG to split critical edges.  This
  // updates dom and loop info.
  AU.addRequired<AliasAnalysis>();
}


/// OptimizeNoopCopyExpression - We have determined that the specified cast
/// instruction is a noop copy (e.g. it's casting from one pointer type to
/// another, int->uint, or int->sbyte on PPC.
///
/// Return true if any changes are made.
static bool OptimizeNoopCopyExpression(CastInst *CI) {
  BasicBlock *DefBB = CI->getParent();
  
  /// InsertedCasts - Only insert a cast in each block once.
  std::map<BasicBlock*, CastInst*> InsertedCasts;
  
  bool MadeChange = false;
  for (Value::use_iterator UI = CI->use_begin(), E = CI->use_end(); 
       UI != E; ) {
    Use &TheUse = UI.getUse();
    Instruction *User = cast<Instruction>(*UI);
    
    // Figure out which BB this cast is used in.  For PHI's this is the
    // appropriate predecessor block.
    BasicBlock *UserBB = User->getParent();
    if (PHINode *PN = dyn_cast<PHINode>(User)) {
      unsigned OpVal = UI.getOperandNo()/2;
      UserBB = PN->getIncomingBlock(OpVal);
    }
    
    // Preincrement use iterator so we don't invalidate it.
    ++UI;
    
    // If this user is in the same block as the cast, don't change the cast.
    if (UserBB == DefBB) continue;
    
    // If we have already inserted a cast into this block, use it.
    CastInst *&InsertedCast = InsertedCasts[UserBB];

    if (!InsertedCast) {
      BasicBlock::iterator InsertPt = UserBB->begin();
      while (isa<PHINode>(InsertPt)) ++InsertPt;
      
      InsertedCast = 
        CastInst::create(CI->getOpcode(), CI->getOperand(0), CI->getType(), "", 
                         InsertPt);
      MadeChange = true;
    }
    
    // Replace a use of the cast with a use of the new casat.
    TheUse = InsertedCast;
  }
  
  // If we removed all uses, nuke the cast.
  if (CI->use_empty())
    CI->eraseFromParent();
  
  return MadeChange;
}

/// InsertGEPComputeCode - Insert code into BB to compute Ptr+PtrOffset,
/// casting to the type of GEPI.
static Instruction *InsertGEPComputeCode(Instruction *&V, BasicBlock *BB,
                                         Instruction *GEPI, Value *Ptr,
                                         Value *PtrOffset) {
  if (V) return V;   // Already computed.
  
  // Figure out the insertion point
  BasicBlock::iterator InsertPt;
  if (BB == GEPI->getParent()) {
    // If GEP is already inserted into BB, insert right after the GEP.
    InsertPt = GEPI;
    ++InsertPt;
  } else {
    // Otherwise, insert at the top of BB, after any PHI nodes
    InsertPt = BB->begin();
    while (isa<PHINode>(InsertPt)) ++InsertPt;
  }
  
  // If Ptr is itself a cast, but in some other BB, emit a copy of the cast into
  // BB so that there is only one value live across basic blocks (the cast 
  // operand).
  if (CastInst *CI = dyn_cast<CastInst>(Ptr))
    if (CI->getParent() != BB && isa<PointerType>(CI->getOperand(0)->getType()))
      Ptr = CastInst::create(CI->getOpcode(), CI->getOperand(0), CI->getType(),
                             "", InsertPt);
  
  // Add the offset, cast it to the right type.
  Ptr = BinaryOperator::createAdd(Ptr, PtrOffset, "", InsertPt);
  // Ptr is an integer type, GEPI is pointer type ==> IntToPtr
  return V = CastInst::create(Instruction::IntToPtr, Ptr, GEPI->getType(), 
                              "", InsertPt);
}

/// ReplaceUsesOfGEPInst - Replace all uses of RepPtr with inserted code to
/// compute its value.  The RepPtr value can be computed with Ptr+PtrOffset. One
/// trivial way of doing this would be to evaluate Ptr+PtrOffset in RepPtr's
/// block, then ReplaceAllUsesWith'ing everything.  However, we would prefer to
/// sink PtrOffset into user blocks where doing so will likely allow us to fold
/// the constant add into a load or store instruction.  Additionally, if a user
/// is a pointer-pointer cast, we look through it to find its users.
static void ReplaceUsesOfGEPInst(Instruction *RepPtr, Value *Ptr, 
                                 Constant *PtrOffset, BasicBlock *DefBB,
                                 GetElementPtrInst *GEPI,
                           std::map<BasicBlock*,Instruction*> &InsertedExprs) {
  while (!RepPtr->use_empty()) {
    Instruction *User = cast<Instruction>(RepPtr->use_back());
    
    // If the user is a Pointer-Pointer cast, recurse. Only BitCast can be
    // used for a Pointer-Pointer cast.
    if (isa<BitCastInst>(User)) {
      ReplaceUsesOfGEPInst(User, Ptr, PtrOffset, DefBB, GEPI, InsertedExprs);
      
      // Drop the use of RepPtr. The cast is dead.  Don't delete it now, else we
      // could invalidate an iterator.
      User->setOperand(0, UndefValue::get(RepPtr->getType()));
      continue;
    }
    
    // If this is a load of the pointer, or a store through the pointer, emit
    // the increment into the load/store block.
    Instruction *NewVal;
    if (isa<LoadInst>(User) ||
        (isa<StoreInst>(User) && User->getOperand(0) != RepPtr)) {
      NewVal = InsertGEPComputeCode(InsertedExprs[User->getParent()], 
                                    User->getParent(), GEPI,
                                    Ptr, PtrOffset);
    } else {
      // If this use is not foldable into the addressing mode, use a version 
      // emitted in the GEP block.
      NewVal = InsertGEPComputeCode(InsertedExprs[DefBB], DefBB, GEPI, 
                                    Ptr, PtrOffset);
    }
    
    if (GEPI->getType() != RepPtr->getType()) {
      BasicBlock::iterator IP = NewVal;
      ++IP;
      // NewVal must be a GEP which must be pointer type, so BitCast
      NewVal = new BitCastInst(NewVal, RepPtr->getType(), "", IP);
    }
    User->replaceUsesOfWith(RepPtr, NewVal);
  }
}


/// OptimizeGEPExpression - Since we are doing basic-block-at-a-time instruction
/// selection, we want to be a bit careful about some things.  In particular, if
/// we have a GEP instruction that is used in a different block than it is
/// defined, the addressing expression of the GEP cannot be folded into loads or
/// stores that use it.  In this case, decompose the GEP and move constant
/// indices into blocks that use it.
static bool OptimizeGEPExpression(GetElementPtrInst *GEPI,
                                  const TargetData *TD) {
  // If this GEP is only used inside the block it is defined in, there is no
  // need to rewrite it.
  bool isUsedOutsideDefBB = false;
  BasicBlock *DefBB = GEPI->getParent();
  for (Value::use_iterator UI = GEPI->use_begin(), E = GEPI->use_end(); 
       UI != E; ++UI) {
    if (cast<Instruction>(*UI)->getParent() != DefBB) {
      isUsedOutsideDefBB = true;
      break;
    }
  }
  if (!isUsedOutsideDefBB) return false;

  // If this GEP has no non-zero constant indices, there is nothing we can do,
  // ignore it.
  bool hasConstantIndex = false;
  bool hasVariableIndex = false;
  for (GetElementPtrInst::op_iterator OI = GEPI->op_begin()+1,
       E = GEPI->op_end(); OI != E; ++OI) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI)) {
      if (CI->getZExtValue()) {
        hasConstantIndex = true;
        break;
      }
    } else {
      hasVariableIndex = true;
    }
  }
  
  // If this is a "GEP X, 0, 0, 0", turn this into a cast.
  if (!hasConstantIndex && !hasVariableIndex) {
    /// The GEP operand must be a pointer, so must its result -> BitCast
    Value *NC = new BitCastInst(GEPI->getOperand(0), GEPI->getType(), 
                             GEPI->getName(), GEPI);
    GEPI->replaceAllUsesWith(NC);
    GEPI->eraseFromParent();
    return true;
  }
  
  // If this is a GEP &Alloca, 0, 0, forward subst the frame index into uses.
  if (!hasConstantIndex && !isa<AllocaInst>(GEPI->getOperand(0)))
    return false;
  
  // Otherwise, decompose the GEP instruction into multiplies and adds.  Sum the
  // constant offset (which we now know is non-zero) and deal with it later.
  uint64_t ConstantOffset = 0;
  const Type *UIntPtrTy = TD->getIntPtrType();
  Value *Ptr = new PtrToIntInst(GEPI->getOperand(0), UIntPtrTy, "", GEPI);
  const Type *Ty = GEPI->getOperand(0)->getType();

  for (GetElementPtrInst::op_iterator OI = GEPI->op_begin()+1,
       E = GEPI->op_end(); OI != E; ++OI) {
    Value *Idx = *OI;
    if (const StructType *StTy = dyn_cast<StructType>(Ty)) {
      unsigned Field = cast<ConstantInt>(Idx)->getZExtValue();
      if (Field)
        ConstantOffset += TD->getStructLayout(StTy)->MemberOffsets[Field];
      Ty = StTy->getElementType(Field);
    } else {
      Ty = cast<SequentialType>(Ty)->getElementType();

      // Handle constant subscripts.
      if (ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
        if (CI->getZExtValue() == 0) continue;
        if (CI->getType()->isSigned())
          ConstantOffset += (int64_t)TD->getTypeSize(Ty)*CI->getSExtValue();
        else
          ConstantOffset += TD->getTypeSize(Ty)*CI->getZExtValue();
        continue;
      }
      
      // Ptr = Ptr + Idx * ElementSize;
      
      // Cast Idx to UIntPtrTy if needed.
      Idx = CastInst::createIntegerCast(Idx, UIntPtrTy, true/*SExt*/, "", GEPI);
      
      uint64_t ElementSize = TD->getTypeSize(Ty);
      // Mask off bits that should not be set.
      ElementSize &= ~0ULL >> (64-UIntPtrTy->getPrimitiveSizeInBits());
      Constant *SizeCst = ConstantInt::get(UIntPtrTy, ElementSize);

      // Multiply by the element size and add to the base.
      Idx = BinaryOperator::createMul(Idx, SizeCst, "", GEPI);
      Ptr = BinaryOperator::createAdd(Ptr, Idx, "", GEPI);
    }
  }
  
  // Make sure that the offset fits in uintptr_t.
  ConstantOffset &= ~0ULL >> (64-UIntPtrTy->getPrimitiveSizeInBits());
  Constant *PtrOffset = ConstantInt::get(UIntPtrTy, ConstantOffset);
  
  // Okay, we have now emitted all of the variable index parts to the BB that
  // the GEP is defined in.  Loop over all of the using instructions, inserting
  // an "add Ptr, ConstantOffset" into each block that uses it and update the
  // instruction to use the newly computed value, making GEPI dead.  When the
  // user is a load or store instruction address, we emit the add into the user
  // block, otherwise we use a canonical version right next to the gep (these 
  // won't be foldable as addresses, so we might as well share the computation).
  
  std::map<BasicBlock*,Instruction*> InsertedExprs;
  ReplaceUsesOfGEPInst(GEPI, Ptr, PtrOffset, DefBB, GEPI, InsertedExprs);
  
  // Finally, the GEP is dead, remove it.
  GEPI->eraseFromParent();
  
  return true;
}


/// SplitEdgeNicely - Split the critical edge from TI to it's specified
/// successor if it will improve codegen.  We only do this if the successor has
/// phi nodes (otherwise critical edges are ok).  If there is already another
/// predecessor of the succ that is empty (and thus has no phi nodes), use it
/// instead of introducing a new block.
static void SplitEdgeNicely(TerminatorInst *TI, unsigned SuccNum, Pass *P) {
  BasicBlock *TIBB = TI->getParent();
  BasicBlock *Dest = TI->getSuccessor(SuccNum);
  assert(isa<PHINode>(Dest->begin()) &&
         "This should only be called if Dest has a PHI!");

  /// TIPHIValues - This array is lazily computed to determine the values of
  /// PHIs in Dest that TI would provide.
  std::vector<Value*> TIPHIValues;
  
  // Check to see if Dest has any blocks that can be used as a split edge for
  // this terminator.
  for (pred_iterator PI = pred_begin(Dest), E = pred_end(Dest); PI != E; ++PI) {
    BasicBlock *Pred = *PI;
    // To be usable, the pred has to end with an uncond branch to the dest.
    BranchInst *PredBr = dyn_cast<BranchInst>(Pred->getTerminator());
    if (!PredBr || !PredBr->isUnconditional() ||
        // Must be empty other than the branch.
        &Pred->front() != PredBr)
      continue;
    
    // Finally, since we know that Dest has phi nodes in it, we have to make
    // sure that jumping to Pred will have the same affect as going to Dest in
    // terms of PHI values.
    PHINode *PN;
    unsigned PHINo = 0;
    bool FoundMatch = true;
    for (BasicBlock::iterator I = Dest->begin();
         (PN = dyn_cast<PHINode>(I)); ++I, ++PHINo) {
      if (PHINo == TIPHIValues.size())
        TIPHIValues.push_back(PN->getIncomingValueForBlock(TIBB));

      // If the PHI entry doesn't work, we can't use this pred.
      if (TIPHIValues[PHINo] != PN->getIncomingValueForBlock(Pred)) {
        FoundMatch = false;
        break;
      }
    }
    
    // If we found a workable predecessor, change TI to branch to Succ.
    if (FoundMatch) {
      Dest->removePredecessor(TIBB);
      TI->setSuccessor(SuccNum, Pred);
      return;
    }
  }
  
  SplitCriticalEdge(TI, SuccNum, P, true);  
}


bool SelectionDAGISel::runOnFunction(Function &Fn) {
  MachineFunction &MF = MachineFunction::construct(&Fn, TLI.getTargetMachine());
  RegMap = MF.getSSARegMap();
  DOUT << "\n\n\n=== " << Fn.getName() << "\n";

  // First, split all critical edges.
  //
  // In this pass we also look for GEP and cast instructions that are used
  // across basic blocks and rewrite them to improve basic-block-at-a-time
  // selection.
  //
  bool MadeChange = true;
  while (MadeChange) {
    MadeChange = false;
  for (Function::iterator BB = Fn.begin(), E = Fn.end(); BB != E; ++BB) {
    // Split all critical edges where the dest block has a PHI.
    TerminatorInst *BBTI = BB->getTerminator();
    if (BBTI->getNumSuccessors() > 1) {
      for (unsigned i = 0, e = BBTI->getNumSuccessors(); i != e; ++i)
        if (isa<PHINode>(BBTI->getSuccessor(i)->begin()) &&
            isCriticalEdge(BBTI, i, true))
          SplitEdgeNicely(BBTI, i, this);
    }
    
    
    for (BasicBlock::iterator BBI = BB->begin(), E = BB->end(); BBI != E; ) {
      Instruction *I = BBI++;
      
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        // If we found an inline asm expession, and if the target knows how to
        // lower it to normal LLVM code, do so now.
        if (isa<InlineAsm>(CI->getCalledValue()))
          if (const TargetAsmInfo *TAI = 
                TLI.getTargetMachine().getTargetAsmInfo()) {
            if (TAI->ExpandInlineAsm(CI))
              BBI = BB->begin();
          }
      } else if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I)) {
        MadeChange |= OptimizeGEPExpression(GEPI, TLI.getTargetData());
      } else if (CastInst *CI = dyn_cast<CastInst>(I)) {
        // If the source of the cast is a constant, then this should have
        // already been constant folded.  The only reason NOT to constant fold
        // it is if something (e.g. LSR) was careful to place the constant
        // evaluation in a block other than then one that uses it (e.g. to hoist
        // the address of globals out of a loop).  If this is the case, we don't
        // want to forward-subst the cast.
        if (isa<Constant>(CI->getOperand(0)))
          continue;
        
        // If this is a noop copy, sink it into user blocks to reduce the number
        // of virtual registers that must be created and coallesced.
        MVT::ValueType SrcVT = TLI.getValueType(CI->getOperand(0)->getType());
        MVT::ValueType DstVT = TLI.getValueType(CI->getType());
        
        // This is an fp<->int conversion?
        if (MVT::isInteger(SrcVT) != MVT::isInteger(DstVT))
          continue;
        
        // If this is an extension, it will be a zero or sign extension, which
        // isn't a noop.
        if (SrcVT < DstVT) continue;
        
        // If these values will be promoted, find out what they will be promoted
        // to.  This helps us consider truncates on PPC as noop copies when they
        // are.
        if (TLI.getTypeAction(SrcVT) == TargetLowering::Promote)
          SrcVT = TLI.getTypeToTransformTo(SrcVT);
        if (TLI.getTypeAction(DstVT) == TargetLowering::Promote)
          DstVT = TLI.getTypeToTransformTo(DstVT);

        // If, after promotion, these are the same types, this is a noop copy.
        if (SrcVT == DstVT)
          MadeChange |= OptimizeNoopCopyExpression(CI);
      }
    }
  }
  }
  
  FunctionLoweringInfo FuncInfo(TLI, Fn, MF);

  for (Function::iterator I = Fn.begin(), E = Fn.end(); I != E; ++I)
    SelectBasicBlock(I, MF, FuncInfo);

  return true;
}

SDOperand SelectionDAGLowering::CopyValueToVirtualRegister(Value *V, 
                                                           unsigned Reg) {
  SDOperand Op = getValue(V);
  assert((Op.getOpcode() != ISD::CopyFromReg ||
          cast<RegisterSDNode>(Op.getOperand(1))->getReg() != Reg) &&
         "Copy from a reg to the same reg!");
  
  // If this type is not legal, we must make sure to not create an invalid
  // register use.
  MVT::ValueType SrcVT = Op.getValueType();
  MVT::ValueType DestVT = TLI.getTypeToTransformTo(SrcVT);
  if (SrcVT == DestVT) {
    return DAG.getCopyToReg(getRoot(), Reg, Op);
  } else if (SrcVT == MVT::Vector) {
    // Handle copies from generic vectors to registers.
    MVT::ValueType PTyElementVT, PTyLegalElementVT;
    unsigned NE = TLI.getPackedTypeBreakdown(cast<PackedType>(V->getType()),
                                             PTyElementVT, PTyLegalElementVT);
    
    // Insert a VBIT_CONVERT of the input vector to a "N x PTyElementVT" 
    // MVT::Vector type.
    Op = DAG.getNode(ISD::VBIT_CONVERT, MVT::Vector, Op,
                     DAG.getConstant(NE, MVT::i32), 
                     DAG.getValueType(PTyElementVT));

    // Loop over all of the elements of the resultant vector,
    // VEXTRACT_VECTOR_ELT'ing them, converting them to PTyLegalElementVT, then
    // copying them into output registers.
    SmallVector<SDOperand, 8> OutChains;
    SDOperand Root = getRoot();
    for (unsigned i = 0; i != NE; ++i) {
      SDOperand Elt = DAG.getNode(ISD::VEXTRACT_VECTOR_ELT, PTyElementVT,
                                  Op, DAG.getConstant(i, TLI.getPointerTy()));
      if (PTyElementVT == PTyLegalElementVT) {
        // Elements are legal.
        OutChains.push_back(DAG.getCopyToReg(Root, Reg++, Elt));
      } else if (PTyLegalElementVT > PTyElementVT) {
        // Elements are promoted.
        if (MVT::isFloatingPoint(PTyLegalElementVT))
          Elt = DAG.getNode(ISD::FP_EXTEND, PTyLegalElementVT, Elt);
        else
          Elt = DAG.getNode(ISD::ANY_EXTEND, PTyLegalElementVT, Elt);
        OutChains.push_back(DAG.getCopyToReg(Root, Reg++, Elt));
      } else {
        // Elements are expanded.
        // The src value is expanded into multiple registers.
        SDOperand Lo = DAG.getNode(ISD::EXTRACT_ELEMENT, PTyLegalElementVT,
                                   Elt, DAG.getConstant(0, TLI.getPointerTy()));
        SDOperand Hi = DAG.getNode(ISD::EXTRACT_ELEMENT, PTyLegalElementVT,
                                   Elt, DAG.getConstant(1, TLI.getPointerTy()));
        OutChains.push_back(DAG.getCopyToReg(Root, Reg++, Lo));
        OutChains.push_back(DAG.getCopyToReg(Root, Reg++, Hi));
      }
    }
    return DAG.getNode(ISD::TokenFactor, MVT::Other,
                       &OutChains[0], OutChains.size());
  } else if (TLI.getTypeAction(SrcVT) == TargetLowering::Promote) {
    // The src value is promoted to the register.
    if (MVT::isFloatingPoint(SrcVT))
      Op = DAG.getNode(ISD::FP_EXTEND, DestVT, Op);
    else
      Op = DAG.getNode(ISD::ANY_EXTEND, DestVT, Op);
    return DAG.getCopyToReg(getRoot(), Reg, Op);
  } else  {
    DestVT = TLI.getTypeToExpandTo(SrcVT);
    unsigned NumVals = TLI.getNumElements(SrcVT);
    if (NumVals == 1)
      return DAG.getCopyToReg(getRoot(), Reg,
                              DAG.getNode(ISD::BIT_CONVERT, DestVT, Op));
    assert(NumVals == 2 && "1 to 4 (and more) expansion not implemented!");
    // The src value is expanded into multiple registers.
    SDOperand Lo = DAG.getNode(ISD::EXTRACT_ELEMENT, DestVT,
                               Op, DAG.getConstant(0, TLI.getPointerTy()));
    SDOperand Hi = DAG.getNode(ISD::EXTRACT_ELEMENT, DestVT,
                               Op, DAG.getConstant(1, TLI.getPointerTy()));
    Op = DAG.getCopyToReg(getRoot(), Reg, Lo);
    return DAG.getCopyToReg(Op, Reg+1, Hi);
  }
}

void SelectionDAGISel::
LowerArguments(BasicBlock *BB, SelectionDAGLowering &SDL,
               std::vector<SDOperand> &UnorderedChains) {
  // If this is the entry block, emit arguments.
  Function &F = *BB->getParent();
  FunctionLoweringInfo &FuncInfo = SDL.FuncInfo;
  SDOperand OldRoot = SDL.DAG.getRoot();
  std::vector<SDOperand> Args = TLI.LowerArguments(F, SDL.DAG);

  unsigned a = 0;
  for (Function::arg_iterator AI = F.arg_begin(), E = F.arg_end();
       AI != E; ++AI, ++a)
    if (!AI->use_empty()) {
      SDL.setValue(AI, Args[a]);

      // If this argument is live outside of the entry block, insert a copy from
      // whereever we got it to the vreg that other BB's will reference it as.
      if (FuncInfo.ValueMap.count(AI)) {
        SDOperand Copy =
          SDL.CopyValueToVirtualRegister(AI, FuncInfo.ValueMap[AI]);
        UnorderedChains.push_back(Copy);
      }
    }

  // Finally, if the target has anything special to do, allow it to do so.
  // FIXME: this should insert code into the DAG!
  EmitFunctionEntryCode(F, SDL.DAG.getMachineFunction());
}

void SelectionDAGISel::BuildSelectionDAG(SelectionDAG &DAG, BasicBlock *LLVMBB,
       std::vector<std::pair<MachineInstr*, unsigned> > &PHINodesToUpdate,
                                         FunctionLoweringInfo &FuncInfo) {
  SelectionDAGLowering SDL(DAG, TLI, FuncInfo);

  std::vector<SDOperand> UnorderedChains;

  // Lower any arguments needed in this block if this is the entry block.
  if (LLVMBB == &LLVMBB->getParent()->front())
    LowerArguments(LLVMBB, SDL, UnorderedChains);

  BB = FuncInfo.MBBMap[LLVMBB];
  SDL.setCurrentBasicBlock(BB);

  // Lower all of the non-terminator instructions.
  for (BasicBlock::iterator I = LLVMBB->begin(), E = --LLVMBB->end();
       I != E; ++I)
    SDL.visit(*I);
  
  // Ensure that all instructions which are used outside of their defining
  // blocks are available as virtual registers.
  for (BasicBlock::iterator I = LLVMBB->begin(), E = LLVMBB->end(); I != E;++I)
    if (!I->use_empty() && !isa<PHINode>(I)) {
      std::map<const Value*, unsigned>::iterator VMI =FuncInfo.ValueMap.find(I);
      if (VMI != FuncInfo.ValueMap.end())
        UnorderedChains.push_back(
                                SDL.CopyValueToVirtualRegister(I, VMI->second));
    }

  // Handle PHI nodes in successor blocks.  Emit code into the SelectionDAG to
  // ensure constants are generated when needed.  Remember the virtual registers
  // that need to be added to the Machine PHI nodes as input.  We cannot just
  // directly add them, because expansion might result in multiple MBB's for one
  // BB.  As such, the start of the BB might correspond to a different MBB than
  // the end.
  //
  TerminatorInst *TI = LLVMBB->getTerminator();

  // Emit constants only once even if used by multiple PHI nodes.
  std::map<Constant*, unsigned> ConstantsOut;
  
  // Vector bool would be better, but vector<bool> is really slow.
  std::vector<unsigned char> SuccsHandled;
  if (TI->getNumSuccessors())
    SuccsHandled.resize(BB->getParent()->getNumBlockIDs());
    
  // Check successor nodes PHI nodes that expect a constant to be available from
  // this block.
  for (unsigned succ = 0, e = TI->getNumSuccessors(); succ != e; ++succ) {
    BasicBlock *SuccBB = TI->getSuccessor(succ);
    if (!isa<PHINode>(SuccBB->begin())) continue;
    MachineBasicBlock *SuccMBB = FuncInfo.MBBMap[SuccBB];
    
    // If this terminator has multiple identical successors (common for
    // switches), only handle each succ once.
    unsigned SuccMBBNo = SuccMBB->getNumber();
    if (SuccsHandled[SuccMBBNo]) continue;
    SuccsHandled[SuccMBBNo] = true;
    
    MachineBasicBlock::iterator MBBI = SuccMBB->begin();
    PHINode *PN;

    // At this point we know that there is a 1-1 correspondence between LLVM PHI
    // nodes and Machine PHI nodes, but the incoming operands have not been
    // emitted yet.
    for (BasicBlock::iterator I = SuccBB->begin();
         (PN = dyn_cast<PHINode>(I)); ++I) {
      // Ignore dead phi's.
      if (PN->use_empty()) continue;
      
      unsigned Reg;
      Value *PHIOp = PN->getIncomingValueForBlock(LLVMBB);
      
      if (Constant *C = dyn_cast<Constant>(PHIOp)) {
        unsigned &RegOut = ConstantsOut[C];
        if (RegOut == 0) {
          RegOut = FuncInfo.CreateRegForValue(C);
          UnorderedChains.push_back(
                           SDL.CopyValueToVirtualRegister(C, RegOut));
        }
        Reg = RegOut;
      } else {
        Reg = FuncInfo.ValueMap[PHIOp];
        if (Reg == 0) {
          assert(isa<AllocaInst>(PHIOp) &&
                 FuncInfo.StaticAllocaMap.count(cast<AllocaInst>(PHIOp)) &&
                 "Didn't codegen value into a register!??");
          Reg = FuncInfo.CreateRegForValue(PHIOp);
          UnorderedChains.push_back(
                           SDL.CopyValueToVirtualRegister(PHIOp, Reg));
        }
      }

      // Remember that this register needs to added to the machine PHI node as
      // the input for this MBB.
      MVT::ValueType VT = TLI.getValueType(PN->getType());
      unsigned NumElements;
      if (VT != MVT::Vector)
        NumElements = TLI.getNumElements(VT);
      else {
        MVT::ValueType VT1,VT2;
        NumElements = 
          TLI.getPackedTypeBreakdown(cast<PackedType>(PN->getType()),
                                     VT1, VT2);
      }
      for (unsigned i = 0, e = NumElements; i != e; ++i)
        PHINodesToUpdate.push_back(std::make_pair(MBBI++, Reg+i));
    }
  }
  ConstantsOut.clear();

  // Turn all of the unordered chains into one factored node.
  if (!UnorderedChains.empty()) {
    SDOperand Root = SDL.getRoot();
    if (Root.getOpcode() != ISD::EntryToken) {
      unsigned i = 0, e = UnorderedChains.size();
      for (; i != e; ++i) {
        assert(UnorderedChains[i].Val->getNumOperands() > 1);
        if (UnorderedChains[i].Val->getOperand(0) == Root)
          break;  // Don't add the root if we already indirectly depend on it.
      }
        
      if (i == e)
        UnorderedChains.push_back(Root);
    }
    DAG.setRoot(DAG.getNode(ISD::TokenFactor, MVT::Other,
                            &UnorderedChains[0], UnorderedChains.size()));
  }

  // Lower the terminator after the copies are emitted.
  SDL.visit(*LLVMBB->getTerminator());

  // Copy over any CaseBlock records that may now exist due to SwitchInst
  // lowering, as well as any jump table information.
  SwitchCases.clear();
  SwitchCases = SDL.SwitchCases;
  JT = SDL.JT;
  
  // Make sure the root of the DAG is up-to-date.
  DAG.setRoot(SDL.getRoot());
}

void SelectionDAGISel::CodeGenAndEmitDAG(SelectionDAG &DAG) {
  // Get alias analysis for load/store combining.
  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();

  // Run the DAG combiner in pre-legalize mode.
  DAG.Combine(false, AA);
  
  DOUT << "Lowered selection DAG:\n";
  DEBUG(DAG.dump());
  
  // Second step, hack on the DAG until it only uses operations and types that
  // the target supports.
  DAG.Legalize();
  
  DOUT << "Legalized selection DAG:\n";
  DEBUG(DAG.dump());
  
  // Run the DAG combiner in post-legalize mode.
  DAG.Combine(true, AA);
  
  if (ViewISelDAGs) DAG.viewGraph();

  // Third, instruction select all of the operations to machine code, adding the
  // code to the MachineBasicBlock.
  InstructionSelectBasicBlock(DAG);
  
  DOUT << "Selected machine code:\n";
  DEBUG(BB->dump());
}  

void SelectionDAGISel::SelectBasicBlock(BasicBlock *LLVMBB, MachineFunction &MF,
                                        FunctionLoweringInfo &FuncInfo) {
  std::vector<std::pair<MachineInstr*, unsigned> > PHINodesToUpdate;
  {
    SelectionDAG DAG(TLI, MF, getAnalysisToUpdate<MachineDebugInfo>());
    CurDAG = &DAG;
  
    // First step, lower LLVM code to some DAG.  This DAG may use operations and
    // types that are not supported by the target.
    BuildSelectionDAG(DAG, LLVMBB, PHINodesToUpdate, FuncInfo);

    // Second step, emit the lowered DAG as machine code.
    CodeGenAndEmitDAG(DAG);
  }
  
  // Next, now that we know what the last MBB the LLVM BB expanded is, update
  // PHI nodes in successors.
  if (SwitchCases.empty() && JT.Reg == 0) {
    for (unsigned i = 0, e = PHINodesToUpdate.size(); i != e; ++i) {
      MachineInstr *PHI = PHINodesToUpdate[i].first;
      assert(PHI->getOpcode() == TargetInstrInfo::PHI &&
             "This is not a machine PHI node that we are updating!");
      PHI->addRegOperand(PHINodesToUpdate[i].second, false);
      PHI->addMachineBasicBlockOperand(BB);
    }
    return;
  }
  
  // If the JumpTable record is filled in, then we need to emit a jump table.
  // Updating the PHI nodes is tricky in this case, since we need to determine
  // whether the PHI is a successor of the range check MBB or the jump table MBB
  if (JT.Reg) {
    assert(SwitchCases.empty() && "Cannot have jump table and lowered switch");
    SelectionDAG SDAG(TLI, MF, getAnalysisToUpdate<MachineDebugInfo>());
    CurDAG = &SDAG;
    SelectionDAGLowering SDL(SDAG, TLI, FuncInfo);
    MachineBasicBlock *RangeBB = BB;
    // Set the current basic block to the mbb we wish to insert the code into
    BB = JT.MBB;
    SDL.setCurrentBasicBlock(BB);
    // Emit the code
    SDL.visitJumpTable(JT);
    SDAG.setRoot(SDL.getRoot());
    CodeGenAndEmitDAG(SDAG);
    // Update PHI Nodes
    for (unsigned pi = 0, pe = PHINodesToUpdate.size(); pi != pe; ++pi) {
      MachineInstr *PHI = PHINodesToUpdate[pi].first;
      MachineBasicBlock *PHIBB = PHI->getParent();
      assert(PHI->getOpcode() == TargetInstrInfo::PHI &&
             "This is not a machine PHI node that we are updating!");
      if (PHIBB == JT.Default) {
        PHI->addRegOperand(PHINodesToUpdate[pi].second, false);
        PHI->addMachineBasicBlockOperand(RangeBB);
      }
      if (BB->succ_end() != std::find(BB->succ_begin(),BB->succ_end(), PHIBB)) {
        PHI->addRegOperand(PHINodesToUpdate[pi].second, false);
        PHI->addMachineBasicBlockOperand(BB);
      }
    }
    return;
  }
  
  // If the switch block involved a branch to one of the actual successors, we
  // need to update PHI nodes in that block.
  for (unsigned i = 0, e = PHINodesToUpdate.size(); i != e; ++i) {
    MachineInstr *PHI = PHINodesToUpdate[i].first;
    assert(PHI->getOpcode() == TargetInstrInfo::PHI &&
           "This is not a machine PHI node that we are updating!");
    if (BB->isSuccessor(PHI->getParent())) {
      PHI->addRegOperand(PHINodesToUpdate[i].second, false);
      PHI->addMachineBasicBlockOperand(BB);
    }
  }
  
  // If we generated any switch lowering information, build and codegen any
  // additional DAGs necessary.
  for (unsigned i = 0, e = SwitchCases.size(); i != e; ++i) {
    SelectionDAG SDAG(TLI, MF, getAnalysisToUpdate<MachineDebugInfo>());
    CurDAG = &SDAG;
    SelectionDAGLowering SDL(SDAG, TLI, FuncInfo);
    
    // Set the current basic block to the mbb we wish to insert the code into
    BB = SwitchCases[i].ThisBB;
    SDL.setCurrentBasicBlock(BB);
    
    // Emit the code
    SDL.visitSwitchCase(SwitchCases[i]);
    SDAG.setRoot(SDL.getRoot());
    CodeGenAndEmitDAG(SDAG);
    
    // Handle any PHI nodes in successors of this chunk, as if we were coming
    // from the original BB before switch expansion.  Note that PHI nodes can
    // occur multiple times in PHINodesToUpdate.  We have to be very careful to
    // handle them the right number of times.
    while ((BB = SwitchCases[i].TrueBB)) {  // Handle LHS and RHS.
      for (MachineBasicBlock::iterator Phi = BB->begin();
           Phi != BB->end() && Phi->getOpcode() == TargetInstrInfo::PHI; ++Phi){
        // This value for this PHI node is recorded in PHINodesToUpdate, get it.
        for (unsigned pn = 0; ; ++pn) {
          assert(pn != PHINodesToUpdate.size() && "Didn't find PHI entry!");
          if (PHINodesToUpdate[pn].first == Phi) {
            Phi->addRegOperand(PHINodesToUpdate[pn].second, false);
            Phi->addMachineBasicBlockOperand(SwitchCases[i].ThisBB);
            break;
          }
        }
      }
      
      // Don't process RHS if same block as LHS.
      if (BB == SwitchCases[i].FalseBB)
        SwitchCases[i].FalseBB = 0;
      
      // If we haven't handled the RHS, do so now.  Otherwise, we're done.
      SwitchCases[i].TrueBB = SwitchCases[i].FalseBB;
      SwitchCases[i].FalseBB = 0;
    }
    assert(SwitchCases[i].TrueBB == 0 && SwitchCases[i].FalseBB == 0);
  }
}


//===----------------------------------------------------------------------===//
/// ScheduleAndEmitDAG - Pick a safe ordering and emit instructions for each
/// target node in the graph.
void SelectionDAGISel::ScheduleAndEmitDAG(SelectionDAG &DAG) {
  if (ViewSchedDAGs) DAG.viewGraph();

  RegisterScheduler::FunctionPassCtor Ctor = RegisterScheduler::getDefault();
  
  if (!Ctor) {
    Ctor = ISHeuristic;
    RegisterScheduler::setDefault(Ctor);
  }
  
  ScheduleDAG *SL = Ctor(this, &DAG, BB);
  BB = SL->Run();
  delete SL;
}


HazardRecognizer *SelectionDAGISel::CreateTargetHazardRecognizer() {
  return new HazardRecognizer();
}

//===----------------------------------------------------------------------===//
// Helper functions used by the generated instruction selector.
//===----------------------------------------------------------------------===//
// Calls to these methods are generated by tblgen.

/// CheckAndMask - The isel is trying to match something like (and X, 255).  If
/// the dag combiner simplified the 255, we still want to match.  RHS is the
/// actual value in the DAG on the RHS of an AND, and DesiredMaskS is the value
/// specified in the .td file (e.g. 255).
bool SelectionDAGISel::CheckAndMask(SDOperand LHS, ConstantSDNode *RHS, 
                                    int64_t DesiredMaskS) {
  uint64_t ActualMask = RHS->getValue();
  uint64_t DesiredMask =DesiredMaskS & MVT::getIntVTBitMask(LHS.getValueType());
  
  // If the actual mask exactly matches, success!
  if (ActualMask == DesiredMask)
    return true;
  
  // If the actual AND mask is allowing unallowed bits, this doesn't match.
  if (ActualMask & ~DesiredMask)
    return false;
  
  // Otherwise, the DAG Combiner may have proven that the value coming in is
  // either already zero or is not demanded.  Check for known zero input bits.
  uint64_t NeededMask = DesiredMask & ~ActualMask;
  if (getTargetLowering().MaskedValueIsZero(LHS, NeededMask))
    return true;
  
  // TODO: check to see if missing bits are just not demanded.

  // Otherwise, this pattern doesn't match.
  return false;
}

/// CheckOrMask - The isel is trying to match something like (or X, 255).  If
/// the dag combiner simplified the 255, we still want to match.  RHS is the
/// actual value in the DAG on the RHS of an OR, and DesiredMaskS is the value
/// specified in the .td file (e.g. 255).
bool SelectionDAGISel::CheckOrMask(SDOperand LHS, ConstantSDNode *RHS, 
                                    int64_t DesiredMaskS) {
  uint64_t ActualMask = RHS->getValue();
  uint64_t DesiredMask =DesiredMaskS & MVT::getIntVTBitMask(LHS.getValueType());
  
  // If the actual mask exactly matches, success!
  if (ActualMask == DesiredMask)
    return true;
  
  // If the actual AND mask is allowing unallowed bits, this doesn't match.
  if (ActualMask & ~DesiredMask)
    return false;
  
  // Otherwise, the DAG Combiner may have proven that the value coming in is
  // either already zero or is not demanded.  Check for known zero input bits.
  uint64_t NeededMask = DesiredMask & ~ActualMask;
  
  uint64_t KnownZero, KnownOne;
  getTargetLowering().ComputeMaskedBits(LHS, NeededMask, KnownZero, KnownOne);
  
  // If all the missing bits in the or are already known to be set, match!
  if ((NeededMask & KnownOne) == NeededMask)
    return true;
  
  // TODO: check to see if missing bits are just not demanded.
  
  // Otherwise, this pattern doesn't match.
  return false;
}


/// SelectInlineAsmMemoryOperands - Calls to this are automatically generated
/// by tblgen.  Others should not call it.
void SelectionDAGISel::
SelectInlineAsmMemoryOperands(std::vector<SDOperand> &Ops, SelectionDAG &DAG) {
  std::vector<SDOperand> InOps;
  std::swap(InOps, Ops);

  Ops.push_back(InOps[0]);  // input chain.
  Ops.push_back(InOps[1]);  // input asm string.

  unsigned i = 2, e = InOps.size();
  if (InOps[e-1].getValueType() == MVT::Flag)
    --e;  // Don't process a flag operand if it is here.
  
  while (i != e) {
    unsigned Flags = cast<ConstantSDNode>(InOps[i])->getValue();
    if ((Flags & 7) != 4 /*MEM*/) {
      // Just skip over this operand, copying the operands verbatim.
      Ops.insert(Ops.end(), InOps.begin()+i, InOps.begin()+i+(Flags >> 3) + 1);
      i += (Flags >> 3) + 1;
    } else {
      assert((Flags >> 3) == 1 && "Memory operand with multiple values?");
      // Otherwise, this is a memory operand.  Ask the target to select it.
      std::vector<SDOperand> SelOps;
      if (SelectInlineAsmMemoryOperand(InOps[i+1], 'm', SelOps, DAG)) {
        cerr << "Could not match memory address.  Inline asm failure!\n";
        exit(1);
      }
      
      // Add this to the output node.
      Ops.push_back(DAG.getTargetConstant(4/*MEM*/ | (SelOps.size() << 3),
                                          MVT::i32));
      Ops.insert(Ops.end(), SelOps.begin(), SelOps.end());
      i += 2;
    }
  }
  
  // Add the flag input back if present.
  if (e != InOps.size())
    Ops.push_back(InOps.back());
}
