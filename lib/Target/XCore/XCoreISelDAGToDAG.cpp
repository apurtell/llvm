//===-- XCoreISelDAGToDAG.cpp - A dag to dag inst selector for XCore ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the XCore target.
//
//===----------------------------------------------------------------------===//

#include "XCore.h"
#include "XCoreTargetMachine.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLowering.h"
using namespace llvm;

/// XCoreDAGToDAGISel - XCore specific code to select XCore machine
/// instructions for SelectionDAG operations.
///
namespace {
  class XCoreDAGToDAGISel : public SelectionDAGISel {
    const XCoreTargetLowering &Lowering;
    const XCoreSubtarget &Subtarget;

  public:
    XCoreDAGToDAGISel(XCoreTargetMachine &TM, CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(TM, OptLevel),
        Lowering(*TM.getTargetLowering()), 
        Subtarget(*TM.getSubtargetImpl()) { }

    SDNode *Select(SDNode *N);
    SDNode *SelectBRIND(SDNode *N);

    /// getI32Imm - Return a target constant with the specified value, of type
    /// i32.
    inline SDValue getI32Imm(unsigned Imm) {
      return CurDAG->getTargetConstant(Imm, MVT::i32);
    }

    inline bool immMskBitp(SDNode *inN) const {
      ConstantSDNode *N = cast<ConstantSDNode>(inN);
      uint32_t value = (uint32_t)N->getZExtValue();
      if (!isMask_32(value)) {
        return false;
      }
      int msksize = 32 - CountLeadingZeros_32(value);
      return (msksize >= 1 && msksize <= 8) ||
              msksize == 16 || msksize == 24 || msksize == 32;
    }

    // Complex Pattern Selectors.
    bool SelectADDRspii(SDValue Addr, SDValue &Base, SDValue &Offset);
    bool SelectADDRdpii(SDValue Addr, SDValue &Base, SDValue &Offset);
    bool SelectADDRcpii(SDValue Addr, SDValue &Base, SDValue &Offset);
    
    virtual const char *getPassName() const {
      return "XCore DAG->DAG Pattern Instruction Selection";
    } 
    
    // Include the pieces autogenerated from the target description.
  #include "XCoreGenDAGISel.inc"
  };
}  // end anonymous namespace

/// createXCoreISelDag - This pass converts a legalized DAG into a 
/// XCore-specific DAG, ready for instruction scheduling.
///
FunctionPass *llvm::createXCoreISelDag(XCoreTargetMachine &TM,
                                       CodeGenOpt::Level OptLevel) {
  return new XCoreDAGToDAGISel(TM, OptLevel);
}

bool XCoreDAGToDAGISel::SelectADDRspii(SDValue Addr, SDValue &Base,
                                       SDValue &Offset) {
  FrameIndexSDNode *FIN = 0;
  if ((FIN = dyn_cast<FrameIndexSDNode>(Addr))) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Offset = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }
  if (Addr.getOpcode() == ISD::ADD) {
    ConstantSDNode *CN = 0;
    if ((FIN = dyn_cast<FrameIndexSDNode>(Addr.getOperand(0)))
      && (CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))
      && (CN->getSExtValue() % 4 == 0 && CN->getSExtValue() >= 0)) {
      // Constant positive word offset from frame index
      Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
      Offset = CurDAG->getTargetConstant(CN->getSExtValue(), MVT::i32);
      return true;
    }
  }
  return false;
}

bool XCoreDAGToDAGISel::SelectADDRdpii(SDValue Addr, SDValue &Base,
                                       SDValue &Offset) {
  if (Addr.getOpcode() == XCoreISD::DPRelativeWrapper) {
    Base = Addr.getOperand(0);
    Offset = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }
  if (Addr.getOpcode() == ISD::ADD) {
    ConstantSDNode *CN = 0;
    if ((Addr.getOperand(0).getOpcode() == XCoreISD::DPRelativeWrapper)
      && (CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))
      && (CN->getSExtValue() % 4 == 0 && CN->getSExtValue() >= 0)) {
      // Constant word offset from a object in the data region
      Base = Addr.getOperand(0).getOperand(0);
      Offset = CurDAG->getTargetConstant(CN->getSExtValue(), MVT::i32);
      return true;
    }
  }
  return false;
}

bool XCoreDAGToDAGISel::SelectADDRcpii(SDValue Addr, SDValue &Base,
                                       SDValue &Offset) {
  if (Addr.getOpcode() == XCoreISD::CPRelativeWrapper) {
    Base = Addr.getOperand(0);
    Offset = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }
  if (Addr.getOpcode() == ISD::ADD) {
    ConstantSDNode *CN = 0;
    if ((Addr.getOperand(0).getOpcode() == XCoreISD::CPRelativeWrapper)
      && (CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))
      && (CN->getSExtValue() % 4 == 0 && CN->getSExtValue() >= 0)) {
      // Constant word offset from a object in the data region
      Base = Addr.getOperand(0).getOperand(0);
      Offset = CurDAG->getTargetConstant(CN->getSExtValue(), MVT::i32);
      return true;
    }
  }
  return false;
}

SDNode *XCoreDAGToDAGISel::Select(SDNode *N) {
  DebugLoc dl = N->getDebugLoc();
  switch (N->getOpcode()) {
  default: break;
  case ISD::Constant: {
    uint64_t Val = cast<ConstantSDNode>(N)->getZExtValue();
    if (immMskBitp(N)) {
      // Transformation function: get the size of a mask
      // Look for the first non-zero bit
      SDValue MskSize = getI32Imm(32 - CountLeadingZeros_32(Val));
      return CurDAG->getMachineNode(XCore::MKMSK_rus, dl,
                                    MVT::i32, MskSize);
    }
    else if (!isUInt<16>(Val)) {
      SDValue CPIdx =
        CurDAG->getTargetConstantPool(ConstantInt::get(
                              Type::getInt32Ty(*CurDAG->getContext()), Val),
                                      TLI.getPointerTy());
      SDNode *node = CurDAG->getMachineNode(XCore::LDWCP_lru6, dl, MVT::i32,
                                            MVT::Other, CPIdx,
                                            CurDAG->getEntryNode());
      MachineSDNode::mmo_iterator MemOp = MF->allocateMemRefsArray(1);
      MemOp[0] = MF->getMachineMemOperand(
        MachinePointerInfo::getConstantPool(), MachineMemOperand::MOLoad, 4, 4);      
      cast<MachineSDNode>(node)->setMemRefs(MemOp, MemOp + 1);
      return node;
    }
    break;
  }
  case XCoreISD::LADD: {
    SDValue Ops[] = { N->getOperand(0), N->getOperand(1),
                        N->getOperand(2) };
    return CurDAG->getMachineNode(XCore::LADD_l5r, dl, MVT::i32, MVT::i32,
                                  Ops);
  }
  case XCoreISD::LSUB: {
    SDValue Ops[] = { N->getOperand(0), N->getOperand(1),
                        N->getOperand(2) };
    return CurDAG->getMachineNode(XCore::LSUB_l5r, dl, MVT::i32, MVT::i32,
                                  Ops);
  }
  case XCoreISD::MACCU: {
    SDValue Ops[] = { N->getOperand(0), N->getOperand(1),
                      N->getOperand(2), N->getOperand(3) };
    return CurDAG->getMachineNode(XCore::MACCU_l4r, dl, MVT::i32, MVT::i32,
                                  Ops);
  }
  case XCoreISD::MACCS: {
    SDValue Ops[] = { N->getOperand(0), N->getOperand(1),
                      N->getOperand(2), N->getOperand(3) };
    return CurDAG->getMachineNode(XCore::MACCS_l4r, dl, MVT::i32, MVT::i32,
                                  Ops);
  }
  case XCoreISD::LMUL: {
    SDValue Ops[] = { N->getOperand(0), N->getOperand(1),
                      N->getOperand(2), N->getOperand(3) };
    return CurDAG->getMachineNode(XCore::LMUL_l6r, dl, MVT::i32, MVT::i32,
                                  Ops);
  }
  case XCoreISD::CRC8: {
    SDValue Ops[] = { N->getOperand(0), N->getOperand(1), N->getOperand(2) };
    return CurDAG->getMachineNode(XCore::CRC8_l4r, dl, MVT::i32, MVT::i32,
                                  Ops);
  }
  case ISD::BRIND:
    if (SDNode *ResNode = SelectBRIND(N))
      return ResNode;
    break;
  // Other cases are autogenerated.
  }
  return SelectCode(N);
}

/// Given a chain return a new chain where any appearance of Old is replaced
/// by New. There must be at most one instruction between Old and Chain and
/// this instruction must be a TokenFactor. Returns an empty SDValue if 
/// these conditions don't hold.
static SDValue
replaceInChain(SelectionDAG *CurDAG, SDValue Chain, SDValue Old, SDValue New)
{
  if (Chain == Old)
    return New;
  if (Chain->getOpcode() != ISD::TokenFactor)
    return SDValue();
  SmallVector<SDValue, 8> Ops;
  bool found = false;
  for (unsigned i = 0, e = Chain->getNumOperands(); i != e; ++i) {
    if (Chain->getOperand(i) == Old) {
      Ops.push_back(New);
      found = true;
    } else {
      Ops.push_back(Chain->getOperand(i));
    }
  }
  if (!found)
    return SDValue();
  return CurDAG->getNode(ISD::TokenFactor, Chain->getDebugLoc(), MVT::Other,
                         &Ops[0], Ops.size());
}

SDNode *XCoreDAGToDAGISel::SelectBRIND(SDNode *N) {
  DebugLoc dl = N->getDebugLoc();
  // (brind (int_xcore_checkevent (addr)))
  SDValue Chain = N->getOperand(0);
  SDValue Addr = N->getOperand(1);
  if (Addr->getOpcode() != ISD::INTRINSIC_W_CHAIN)
    return 0;
  unsigned IntNo = cast<ConstantSDNode>(Addr->getOperand(1))->getZExtValue();
  if (IntNo != Intrinsic::xcore_checkevent)
    return 0;
  SDValue nextAddr = Addr->getOperand(2);
  SDValue CheckEventChainOut(Addr.getNode(), 1);
  if (!CheckEventChainOut.use_empty()) {
    // If the chain out of the checkevent intrinsic is an operand of the
    // indirect branch or used in a TokenFactor which is the operand of the
    // indirect branch then build a new chain which uses the chain coming into
    // the checkevent intrinsic instead.
    SDValue CheckEventChainIn = Addr->getOperand(0);
    SDValue NewChain = replaceInChain(CurDAG, Chain, CheckEventChainOut,
                                      CheckEventChainIn);
    if (!NewChain.getNode())
      return 0;
    Chain = NewChain;
  }
  // Enable events on the thread using setsr 1 and then disable them immediately
  // after with clrsr 1. If any resources owned by the thread are ready an event
  // will be taken. If no resource is ready we branch to the address which was
  // the operand to the checkevent intrinsic.
  SDValue constOne = getI32Imm(1);
  SDValue Glue =
    SDValue(CurDAG->getMachineNode(XCore::SETSR_branch_u6, dl, MVT::Glue,
                                   constOne, Chain), 0);
  Glue =
    SDValue(CurDAG->getMachineNode(XCore::CLRSR_branch_u6, dl, MVT::Glue,
                                   constOne, Glue), 0);
  if (nextAddr->getOpcode() == XCoreISD::PCRelativeWrapper &&
      nextAddr->getOperand(0)->getOpcode() == ISD::TargetBlockAddress) {
    return CurDAG->SelectNodeTo(N, XCore::BRFU_lu6, MVT::Other,
                                nextAddr->getOperand(0), Glue);
  }
  return CurDAG->SelectNodeTo(N, XCore::BAU_1r, MVT::Other, nextAddr, Glue);
}
