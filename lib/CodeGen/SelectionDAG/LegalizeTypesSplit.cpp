//===-- LegalizeTypesSplit.cpp - Vector Splitting for LegalizeTypes -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by Chris Lattner and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements vector splitting support for LegalizeTypes.  Vector
// splitting is the act of changing a computation in an invalid vector type to
// be a computation in multiple vectors of a smaller type.  For example,
// implementing <128 x f32> operations in terms of two <64 x f32> operations.
//
//===----------------------------------------------------------------------===//

#include "LegalizeTypes.h"
using namespace llvm;

/// GetSplitDestVTs - Compute the VTs needed for the low/hi parts of a vector
/// type that needs to be split.  This handles non-power of two vectors.
static void GetSplitDestVTs(MVT::ValueType InVT,
                            MVT::ValueType &Lo, MVT::ValueType &Hi) {
  MVT::ValueType NewEltVT = MVT::getVectorElementType(InVT);
  unsigned NumElements = MVT::getVectorNumElements(InVT);
  if ((NumElements & (NumElements-1)) == 0) {  // Simple power of two vector.
    NumElements >>= 1;
    Lo = Hi =  MVT::getVectorType(NewEltVT, NumElements);
  } else {                                     // Non-power-of-two vectors.
    unsigned NewNumElts_Lo = 1 << Log2_32(NumElements-1);
    unsigned NewNumElts_Hi = NumElements - NewNumElts_Lo;
    Lo = MVT::getVectorType(NewEltVT, NewNumElts_Lo);
    Hi = MVT::getVectorType(NewEltVT, NewNumElts_Hi);
  }
}


//===----------------------------------------------------------------------===//
//  Result Vector Splitting
//===----------------------------------------------------------------------===//

/// SplitResult - This method is called when the specified result of the
/// specified node is found to need vector splitting.  At this point, the node
/// may also have invalid operands or may have other results that need
/// legalization, we just know that (at least) one result needs vector
/// splitting.
void DAGTypeLegalizer::SplitResult(SDNode *N, unsigned ResNo) {
  DEBUG(cerr << "Expand node result: "; N->dump(&DAG); cerr << "\n");
  SDOperand Lo, Hi;
  
#if 0
  // See if the target wants to custom expand this node.
  if (TLI.getOperationAction(N->getOpcode(), N->getValueType(0)) == 
      TargetLowering::Custom) {
    // If the target wants to, allow it to lower this itself.
    if (SDNode *P = TLI.ExpandOperationResult(N, DAG)) {
      // Everything that once used N now uses P.  We are guaranteed that the
      // result value types of N and the result value types of P match.
      ReplaceNodeWith(N, P);
      return;
    }
  }
#endif
  
  switch (N->getOpcode()) {
  default:
#ifndef NDEBUG
    cerr << "SplitResult #" << ResNo << ": ";
    N->dump(&DAG); cerr << "\n";
#endif
    assert(0 && "Do not know how to split the result of this operator!");
    abort();
    
  case ISD::UNDEF:            SplitRes_UNDEF(N, Lo, Hi); break;
  case ISD::LOAD:             SplitRes_LOAD(cast<LoadSDNode>(N), Lo, Hi); break;
  case ISD::BUILD_PAIR:       SplitRes_BUILD_PAIR(N, Lo, Hi); break;
  case ISD::INSERT_VECTOR_ELT:SplitRes_INSERT_VECTOR_ELT(N, Lo, Hi); break;
  case ISD::VECTOR_SHUFFLE:   SplitRes_VECTOR_SHUFFLE(N, Lo, Hi); break;
  case ISD::BUILD_VECTOR:     SplitRes_BUILD_VECTOR(N, Lo, Hi); break;
  case ISD::CONCAT_VECTORS:   SplitRes_CONCAT_VECTORS(N, Lo, Hi); break;
  case ISD::BIT_CONVERT:      SplitRes_BIT_CONVERT(N, Lo, Hi); break;
  case ISD::CTTZ:
  case ISD::CTLZ:
  case ISD::CTPOP:
  case ISD::FNEG:
  case ISD::FABS:
  case ISD::FSQRT:
  case ISD::FSIN:
  case ISD::FCOS:
  case ISD::FP_TO_SINT:
  case ISD::FP_TO_UINT:
  case ISD::SINT_TO_FP:
  case ISD::UINT_TO_FP:       SplitRes_UnOp(N, Lo, Hi); break;
  case ISD::ADD:
  case ISD::SUB:
  case ISD::MUL:
  case ISD::FADD:
  case ISD::FSUB:
  case ISD::FMUL:
  case ISD::SDIV:
  case ISD::UDIV:
  case ISD::FDIV:
  case ISD::FPOW:
  case ISD::AND:
  case ISD::OR:
  case ISD::XOR:
  case ISD::UREM:
  case ISD::SREM:
  case ISD::FREM:             SplitRes_BinOp(N, Lo, Hi); break;
  case ISD::FPOWI:            SplitRes_FPOWI(N, Lo, Hi); break;
  case ISD::SELECT:           SplitRes_SELECT(N, Lo, Hi); break;
  }
  
  // If Lo/Hi is null, the sub-method took care of registering results etc.
  if (Lo.Val)
    SetSplitOp(SDOperand(N, ResNo), Lo, Hi);
}

void DAGTypeLegalizer::SplitRes_UNDEF(SDNode *N, SDOperand &Lo, SDOperand &Hi) {
  MVT::ValueType LoVT, HiVT;
  GetSplitDestVTs(N->getValueType(0), LoVT, HiVT);

  Lo = DAG.getNode(ISD::UNDEF, LoVT);
  Hi = DAG.getNode(ISD::UNDEF, HiVT);
}

void DAGTypeLegalizer::SplitRes_LOAD(LoadSDNode *LD, 
                                     SDOperand &Lo, SDOperand &Hi) {
  MVT::ValueType LoVT, HiVT;
  GetSplitDestVTs(LD->getValueType(0), LoVT, HiVT);
  
  SDOperand Ch = LD->getChain();
  SDOperand Ptr = LD->getBasePtr();
  const Value *SV = LD->getSrcValue();
  int SVOffset = LD->getSrcValueOffset();
  unsigned Alignment = LD->getAlignment();
  bool isVolatile = LD->isVolatile();
  
  Lo = DAG.getLoad(LoVT, Ch, Ptr, SV, SVOffset, isVolatile, Alignment);
  unsigned IncrementSize = MVT::getSizeInBits(LoVT)/8;
  Ptr = DAG.getNode(ISD::ADD, Ptr.getValueType(), Ptr,
                    getIntPtrConstant(IncrementSize));
  SVOffset += IncrementSize;
  Alignment = MinAlign(Alignment, IncrementSize);
  Hi = DAG.getLoad(HiVT, Ch, Ptr, SV, SVOffset, isVolatile, Alignment);
  
  // Build a factor node to remember that this load is independent of the
  // other one.
  SDOperand TF = DAG.getNode(ISD::TokenFactor, MVT::Other, Lo.getValue(1),
                             Hi.getValue(1));
  
  // Legalized the chain result - switch anything that used the old chain to
  // use the new one.
  ReplaceValueWith(SDOperand(LD, 1), TF);
}

void DAGTypeLegalizer::SplitRes_BUILD_PAIR(SDNode *N, SDOperand &Lo,
                                           SDOperand &Hi) {
  Lo = N->getOperand(0);
  Hi = N->getOperand(1);
}

void DAGTypeLegalizer::SplitRes_INSERT_VECTOR_ELT(SDNode *N, SDOperand &Lo,
                                                  SDOperand &Hi) {
  GetSplitOp(N->getOperand(0), Lo, Hi);
  unsigned Index = cast<ConstantSDNode>(N->getOperand(2))->getValue();
  SDOperand ScalarOp = N->getOperand(1);
  unsigned LoNumElts = MVT::getVectorNumElements(Lo.getValueType());
  if (Index < LoNumElts)
    Lo = DAG.getNode(ISD::INSERT_VECTOR_ELT, Lo.getValueType(), Lo, ScalarOp,
                     N->getOperand(2));
  else
    Hi = DAG.getNode(ISD::INSERT_VECTOR_ELT, Hi.getValueType(), Hi, ScalarOp,
                     DAG.getConstant(Index - LoNumElts, TLI.getPointerTy()));
  
}

void DAGTypeLegalizer::SplitRes_VECTOR_SHUFFLE(SDNode *N, 
                                               SDOperand &Lo, SDOperand &Hi) {
  // Build the low part.
  SDOperand Mask = N->getOperand(2);
  SmallVector<SDOperand, 16> Ops;
  MVT::ValueType PtrVT = TLI.getPointerTy();
  
  MVT::ValueType LoVT, HiVT;
  GetSplitDestVTs(N->getValueType(0), LoVT, HiVT);
  MVT::ValueType EltVT = MVT::getVectorElementType(LoVT);
  unsigned LoNumElts = MVT::getVectorNumElements(LoVT);
  unsigned NumElements = Mask.getNumOperands();

  // Insert all of the elements from the input that are needed.  We use 
  // buildvector of extractelement here because the input vectors will have
  // to be legalized, so this makes the code simpler.
  for (unsigned i = 0; i != LoNumElts; ++i) {
    unsigned Idx = cast<ConstantSDNode>(Mask.getOperand(i))->getValue();
    SDOperand InVec = N->getOperand(0);
    if (Idx >= NumElements) {
      InVec = N->getOperand(1);
      Idx -= NumElements;
    }
    Ops.push_back(DAG.getNode(ISD::EXTRACT_VECTOR_ELT, EltVT, InVec,
                              DAG.getConstant(Idx, PtrVT)));
  }
  Lo = DAG.getNode(ISD::BUILD_VECTOR, LoVT, &Ops[0], Ops.size());
  Ops.clear();
  
  for (unsigned i = LoNumElts; i != NumElements; ++i) {
    unsigned Idx = cast<ConstantSDNode>(Mask.getOperand(i))->getValue();
    SDOperand InVec = N->getOperand(0);
    if (Idx >= NumElements) {
      InVec = N->getOperand(1);
      Idx -= NumElements;
    }
    Ops.push_back(DAG.getNode(ISD::EXTRACT_VECTOR_ELT, EltVT, InVec,
                              DAG.getConstant(Idx, PtrVT)));
  }
  Hi = DAG.getNode(ISD::BUILD_VECTOR, HiVT, &Ops[0], Ops.size());
}

void DAGTypeLegalizer::SplitRes_BUILD_VECTOR(SDNode *N, SDOperand &Lo, 
                                             SDOperand &Hi) {
  MVT::ValueType LoVT, HiVT;
  GetSplitDestVTs(N->getValueType(0), LoVT, HiVT);
  unsigned LoNumElts = MVT::getVectorNumElements(LoVT);
  SmallVector<SDOperand, 8> LoOps(N->op_begin(), N->op_begin()+LoNumElts);
  Lo = DAG.getNode(ISD::BUILD_VECTOR, LoVT, &LoOps[0], LoOps.size());
  
  SmallVector<SDOperand, 8> HiOps(N->op_begin()+LoNumElts, N->op_end());
  Hi = DAG.getNode(ISD::BUILD_VECTOR, HiVT, &HiOps[0], HiOps.size());
}

void DAGTypeLegalizer::SplitRes_CONCAT_VECTORS(SDNode *N, 
                                               SDOperand &Lo, SDOperand &Hi) {
  // FIXME: Handle non-power-of-two vectors?
  unsigned NumSubvectors = N->getNumOperands() / 2;
  if (NumSubvectors == 1) {
    Lo = N->getOperand(0);
    Hi = N->getOperand(1);
    return;
  }

  MVT::ValueType LoVT, HiVT;
  GetSplitDestVTs(N->getValueType(0), LoVT, HiVT);

  SmallVector<SDOperand, 8> LoOps(N->op_begin(), N->op_begin()+NumSubvectors);
  Lo = DAG.getNode(ISD::CONCAT_VECTORS, LoVT, &LoOps[0], LoOps.size());
    
  SmallVector<SDOperand, 8> HiOps(N->op_begin()+NumSubvectors, N->op_end());
  Hi = DAG.getNode(ISD::CONCAT_VECTORS, HiVT, &HiOps[0], HiOps.size());
}

void DAGTypeLegalizer::SplitRes_BIT_CONVERT(SDNode *N, 
                                            SDOperand &Lo, SDOperand &Hi) {
  // We know the result is a vector.  The input may be either a vector or a
  // scalar value.
  SDOperand InOp = N->getOperand(0);
  if (MVT::isVector(InOp.getValueType()) &&
      MVT::getVectorNumElements(InOp.getValueType()) != 1) {
    // If this is a vector, split the vector and convert each of the pieces now.
    GetSplitOp(InOp, Lo, Hi);
    
    MVT::ValueType LoVT, HiVT;
    GetSplitDestVTs(N->getValueType(0), LoVT, HiVT);

    Lo = DAG.getNode(ISD::BIT_CONVERT, LoVT, Lo);
    Hi = DAG.getNode(ISD::BIT_CONVERT, HiVT, Hi);
    return;
  }
  
  // Lower the bit-convert to a store/load from the stack, then split the load.
  SDOperand Op = CreateStackStoreLoad(N->getOperand(0), N->getValueType(0));
  SplitRes_LOAD(cast<LoadSDNode>(Op.Val), Lo, Hi);
}

void DAGTypeLegalizer::SplitRes_BinOp(SDNode *N, SDOperand &Lo, SDOperand &Hi) {
  SDOperand LHSLo, LHSHi;
  GetSplitOp(N->getOperand(0), LHSLo, LHSHi);
  SDOperand RHSLo, RHSHi;
  GetSplitOp(N->getOperand(1), RHSLo, RHSHi);
  
  Lo = DAG.getNode(N->getOpcode(), LHSLo.getValueType(), LHSLo, RHSLo);
  Hi = DAG.getNode(N->getOpcode(), LHSHi.getValueType(), LHSHi, RHSHi);
}

void DAGTypeLegalizer::SplitRes_UnOp(SDNode *N, SDOperand &Lo, SDOperand &Hi) {
  // Get the dest types.  This doesn't always match input types, e.g. int_to_fp.
  MVT::ValueType LoVT, HiVT;
  GetSplitDestVTs(N->getValueType(0), LoVT, HiVT);

  GetSplitOp(N->getOperand(0), Lo, Hi);
  Lo = DAG.getNode(N->getOpcode(), LoVT, Lo);
  Hi = DAG.getNode(N->getOpcode(), HiVT, Hi);
}

void DAGTypeLegalizer::SplitRes_FPOWI(SDNode *N, SDOperand &Lo, SDOperand &Hi) {
  GetSplitOp(N->getOperand(0), Lo, Hi);
  Lo = DAG.getNode(ISD::FPOWI, Lo.getValueType(), Lo, N->getOperand(1));
  Hi = DAG.getNode(ISD::FPOWI, Lo.getValueType(), Hi, N->getOperand(1));
}


void DAGTypeLegalizer::SplitRes_SELECT(SDNode *N, SDOperand &Lo, SDOperand &Hi){
  SDOperand LL, LH, RL, RH;
  GetSplitOp(N->getOperand(1), LL, LH);
  GetSplitOp(N->getOperand(2), RL, RH);
  
  SDOperand Cond = N->getOperand(0);
  Lo = DAG.getNode(ISD::SELECT, LL.getValueType(), Cond, LL, RL);
  Hi = DAG.getNode(ISD::SELECT, LH.getValueType(), Cond, LH, RH);
}


//===----------------------------------------------------------------------===//
//  Operand Vector Splitting
//===----------------------------------------------------------------------===//

/// SplitOperand - This method is called when the specified operand of the
/// specified node is found to need vector splitting.  At this point, all of the
/// result types of the node are known to be legal, but other operands of the
/// node may need legalization as well as the specified one.
bool DAGTypeLegalizer::SplitOperand(SDNode *N, unsigned OpNo) {
  DEBUG(cerr << "Split node operand: "; N->dump(&DAG); cerr << "\n");
  SDOperand Res(0, 0);
  
#if 0
  if (TLI.getOperationAction(N->getOpcode(), N->getValueType(0)) == 
      TargetLowering::Custom)
    Res = TLI.LowerOperation(SDOperand(N, 0), DAG);
#endif
  
  if (Res.Val == 0) {
    switch (N->getOpcode()) {
    default:
#ifndef NDEBUG
      cerr << "SplitOperand Op #" << OpNo << ": ";
      N->dump(&DAG); cerr << "\n";
#endif
      assert(0 && "Do not know how to split this operator's operand!");
      abort();
    case ISD::STORE: Res = SplitOp_STORE(cast<StoreSDNode>(N), OpNo); break;
    case ISD::RET:   Res = SplitOp_RET(N, OpNo); break;
    }
  }
  
  // If the result is null, the sub-method took care of registering results etc.
  if (!Res.Val) return false;
  
  // If the result is N, the sub-method updated N in place.  Check to see if any
  // operands are new, and if so, mark them.
  if (Res.Val == N) {
    // Mark N as new and remark N and its operands.  This allows us to correctly
    // revisit N if it needs another step of promotion and allows us to visit
    // any new operands to N.
    N->setNodeId(NewNode);
    MarkNewNodes(N);
    return true;
  }
  
  assert(Res.getValueType() == N->getValueType(0) && N->getNumValues() == 1 &&
         "Invalid operand expansion");
  
  ReplaceValueWith(SDOperand(N, 0), Res);
  return false;
}

SDOperand DAGTypeLegalizer::SplitOp_STORE(StoreSDNode *N, unsigned OpNo) {
  assert(OpNo == 1 && "Can only split the stored value");
  
  SDOperand Ch  = N->getChain();
  SDOperand Ptr = N->getBasePtr();
  int SVOffset = N->getSrcValueOffset();
  unsigned Alignment = N->getAlignment();
  bool isVol = N->isVolatile();
  SDOperand Lo, Hi;
  GetSplitOp(N->getOperand(1), Lo, Hi);

  unsigned IncrementSize = MVT::getSizeInBits(Lo.getValueType())/8;

  Lo = DAG.getStore(Ch, Lo, Ptr, N->getSrcValue(), SVOffset, isVol, Alignment);
  
  // Increment the pointer to the other half.
  Ptr = DAG.getNode(ISD::ADD, Ptr.getValueType(), Ptr,
                    getIntPtrConstant(IncrementSize));
  
  Hi = DAG.getStore(Ch, Hi, Ptr, N->getSrcValue(), SVOffset+IncrementSize,
                    isVol, MinAlign(Alignment, IncrementSize));
  return DAG.getNode(ISD::TokenFactor, MVT::Other, Lo, Hi);
}

SDOperand DAGTypeLegalizer::SplitOp_RET(SDNode *N, unsigned OpNo) {
  assert(N->getNumOperands() == 3 &&"Can only handle ret of one vector so far");
  // FIXME: Returns of gcc generic vectors larger than a legal vector
  // type should be returned by reference!
  SDOperand Lo, Hi;
  GetSplitOp(N->getOperand(1), Lo, Hi);

  SDOperand Chain = N->getOperand(0);  // The chain.
  SDOperand Sign = N->getOperand(2);  // Signness
  
  return DAG.getNode(ISD::RET, MVT::Other, Chain, Lo, Sign, Hi, Sign);
}
