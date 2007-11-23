//===-- ConstantFolding.cpp - Analyze constant folding possibilities ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This family of functions determines the possibility of performing constant
// folding.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/MathExtras.h"
#include <cerrno>
#include <cmath>
using namespace llvm;

//===----------------------------------------------------------------------===//
// Constant Folding internal helper functions
//===----------------------------------------------------------------------===//

/// IsConstantOffsetFromGlobal - If this constant is actually a constant offset
/// from a global, return the global and the constant.  Because of
/// constantexprs, this function is recursive.
static bool IsConstantOffsetFromGlobal(Constant *C, GlobalValue *&GV,
                                       int64_t &Offset, const TargetData &TD) {
  // Trivial case, constant is the global.
  if ((GV = dyn_cast<GlobalValue>(C))) {
    Offset = 0;
    return true;
  }
  
  // Otherwise, if this isn't a constant expr, bail out.
  ConstantExpr *CE = dyn_cast<ConstantExpr>(C);
  if (!CE) return false;
  
  // Look through ptr->int and ptr->ptr casts.
  if (CE->getOpcode() == Instruction::PtrToInt ||
      CE->getOpcode() == Instruction::BitCast)
    return IsConstantOffsetFromGlobal(CE->getOperand(0), GV, Offset, TD);
  
  // i32* getelementptr ([5 x i32]* @a, i32 0, i32 5)    
  if (CE->getOpcode() == Instruction::GetElementPtr) {
    // Cannot compute this if the element type of the pointer is missing size
    // info.
    if (!cast<PointerType>(CE->getOperand(0)->getType())->getElementType()->isSized())
      return false;
    
    // If the base isn't a global+constant, we aren't either.
    if (!IsConstantOffsetFromGlobal(CE->getOperand(0), GV, Offset, TD))
      return false;
    
    // Otherwise, add any offset that our operands provide.
    gep_type_iterator GTI = gep_type_begin(CE);
    for (unsigned i = 1, e = CE->getNumOperands(); i != e; ++i, ++GTI) {
      ConstantInt *CI = dyn_cast<ConstantInt>(CE->getOperand(i));
      if (!CI) return false;  // Index isn't a simple constant?
      if (CI->getZExtValue() == 0) continue;  // Not adding anything.
      
      if (const StructType *ST = dyn_cast<StructType>(*GTI)) {
        // N = N + Offset
        Offset += TD.getStructLayout(ST)->getElementOffset(CI->getZExtValue());
      } else {
        const SequentialType *SQT = cast<SequentialType>(*GTI);
        Offset += TD.getABITypeSize(SQT->getElementType())*CI->getSExtValue();
      }
    }
    return true;
  }
  
  return false;
}


/// SymbolicallyEvaluateBinop - One of Op0/Op1 is a constant expression.
/// Attempt to symbolically evaluate the result of  a binary operator merging
/// these together.  If target data info is available, it is provided as TD, 
/// otherwise TD is null.
static Constant *SymbolicallyEvaluateBinop(unsigned Opc, Constant *Op0,
                                           Constant *Op1, const TargetData *TD){
  // SROA
  
  // Fold (and 0xffffffff00000000, (shl x, 32)) -> shl.
  // Fold (lshr (or X, Y), 32) -> (lshr [X/Y], 32) if one doesn't contribute
  // bits.
  
  
  // If the constant expr is something like &A[123] - &A[4].f, fold this into a
  // constant.  This happens frequently when iterating over a global array.
  if (Opc == Instruction::Sub && TD) {
    GlobalValue *GV1, *GV2;
    int64_t Offs1, Offs2;
    
    if (IsConstantOffsetFromGlobal(Op0, GV1, Offs1, *TD))
      if (IsConstantOffsetFromGlobal(Op1, GV2, Offs2, *TD) &&
          GV1 == GV2) {
        // (&GV+C1) - (&GV+C2) -> C1-C2, pointer arithmetic cannot overflow.
        return ConstantInt::get(Op0->getType(), Offs1-Offs2);
      }
  }
    
  // TODO: Fold icmp setne/seteq as well.
  return 0;
}

/// SymbolicallyEvaluateGEP - If we can symbolically evaluate the specified GEP
/// constant expression, do so.
static Constant *SymbolicallyEvaluateGEP(Constant** Ops, unsigned NumOps,
                                         const Type *ResultTy,
                                         const TargetData *TD) {
  Constant *Ptr = Ops[0];
  if (!cast<PointerType>(Ptr->getType())->getElementType()->isSized())
    return 0;
  
  if (TD && Ptr->isNullValue()) {
    // If this is a constant expr gep that is effectively computing an
    // "offsetof", fold it into 'cast int Size to T*' instead of 'gep 0, 0, 12'
    bool isFoldableGEP = true;
    for (unsigned i = 1; i != NumOps; ++i)
      if (!isa<ConstantInt>(Ops[i])) {
        isFoldableGEP = false;
        break;
      }
    if (isFoldableGEP) {
      uint64_t Offset = TD->getIndexedOffset(Ptr->getType(),
                                             (Value**)Ops+1, NumOps-1);
      Constant *C = ConstantInt::get(TD->getIntPtrType(), Offset);
      return ConstantExpr::getIntToPtr(C, ResultTy);
    }
  }
  
  return 0;
}


//===----------------------------------------------------------------------===//
// Constant Folding public APIs
//===----------------------------------------------------------------------===//


/// ConstantFoldInstruction - Attempt to constant fold the specified
/// instruction.  If successful, the constant result is returned, if not, null
/// is returned.  Note that this function can only fail when attempting to fold
/// instructions like loads and stores, which have no constant expression form.
///
Constant *llvm::ConstantFoldInstruction(Instruction *I, const TargetData *TD) {
  if (PHINode *PN = dyn_cast<PHINode>(I)) {
    if (PN->getNumIncomingValues() == 0)
      return Constant::getNullValue(PN->getType());

    Constant *Result = dyn_cast<Constant>(PN->getIncomingValue(0));
    if (Result == 0) return 0;

    // Handle PHI nodes specially here...
    for (unsigned i = 1, e = PN->getNumIncomingValues(); i != e; ++i)
      if (PN->getIncomingValue(i) != Result && PN->getIncomingValue(i) != PN)
        return 0;   // Not all the same incoming constants...

    // If we reach here, all incoming values are the same constant.
    return Result;
  }

  // Scan the operand list, checking to see if they are all constants, if so,
  // hand off to ConstantFoldInstOperands.
  SmallVector<Constant*, 8> Ops;
  for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i)
    if (Constant *Op = dyn_cast<Constant>(I->getOperand(i)))
      Ops.push_back(Op);
    else
      return 0;  // All operands not constant!

  return ConstantFoldInstOperands(I, &Ops[0], Ops.size(), TD);
}

/// ConstantFoldInstOperands - Attempt to constant fold an instruction with the
/// specified opcode and operands.  If successful, the constant result is
/// returned, if not, null is returned.  Note that this function can fail when
/// attempting to fold instructions like loads and stores, which have no
/// constant expression form.
///
Constant *llvm::ConstantFoldInstOperands(const Instruction* I, 
                                         Constant** Ops, unsigned NumOps,
                                         const TargetData *TD) {
  unsigned Opc = I->getOpcode();
  const Type *DestTy = I->getType();

  // Handle easy binops first.
  if (isa<BinaryOperator>(I)) {
    if (isa<ConstantExpr>(Ops[0]) || isa<ConstantExpr>(Ops[1]))
      if (Constant *C = SymbolicallyEvaluateBinop(I->getOpcode(), Ops[0],
                                                  Ops[1], TD))
        return C;
    
    return ConstantExpr::get(Opc, Ops[0], Ops[1]);
  }
  
  switch (Opc) {
  default: return 0;
  case Instruction::Call:
    if (Function *F = dyn_cast<Function>(Ops[0]))
      if (canConstantFoldCallTo(F))
        return ConstantFoldCall(F, Ops+1, NumOps-1);
    return 0;
  case Instruction::ICmp:
  case Instruction::FCmp:
    return ConstantExpr::getCompare(cast<CmpInst>(I)->getPredicate(), Ops[0], 
                                    Ops[1]);
  case Instruction::PtrToInt:
    // If the input is a inttoptr, eliminate the pair.  This requires knowing
    // the width of a pointer, so it can't be done in ConstantExpr::getCast.
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Ops[0])) {
      if (TD && CE->getOpcode() == Instruction::IntToPtr) {
        Constant *Input = CE->getOperand(0);
        unsigned InWidth = Input->getType()->getPrimitiveSizeInBits();
        Constant *Mask = 
          ConstantInt::get(APInt::getLowBitsSet(InWidth,
                                                TD->getPointerSizeInBits()));
        Input = ConstantExpr::getAnd(Input, Mask);
        // Do a zext or trunc to get to the dest size.
        return ConstantExpr::getIntegerCast(Input, I->getType(), false);
      }
    }
    // FALL THROUGH.
  case Instruction::IntToPtr:
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::BitCast:
    return ConstantExpr::getCast(Opc, Ops[0], DestTy);
  case Instruction::Select:
    return ConstantExpr::getSelect(Ops[0], Ops[1], Ops[2]);
  case Instruction::ExtractElement:
    return ConstantExpr::getExtractElement(Ops[0], Ops[1]);
  case Instruction::InsertElement:
    return ConstantExpr::getInsertElement(Ops[0], Ops[1], Ops[2]);
  case Instruction::ShuffleVector:
    return ConstantExpr::getShuffleVector(Ops[0], Ops[1], Ops[2]);
  case Instruction::GetElementPtr:
    if (Constant *C = SymbolicallyEvaluateGEP(Ops, NumOps, I->getType(), TD))
      return C;
    
    return ConstantExpr::getGetElementPtr(Ops[0], Ops+1, NumOps-1);
  }
}

/// ConstantFoldLoadThroughGEPConstantExpr - Given a constant and a
/// getelementptr constantexpr, return the constant value being addressed by the
/// constant expression, or null if something is funny and we can't decide.
Constant *llvm::ConstantFoldLoadThroughGEPConstantExpr(Constant *C, 
                                                       ConstantExpr *CE) {
  if (CE->getOperand(1) != Constant::getNullValue(CE->getOperand(1)->getType()))
    return 0;  // Do not allow stepping over the value!
  
  // Loop over all of the operands, tracking down which value we are
  // addressing...
  gep_type_iterator I = gep_type_begin(CE), E = gep_type_end(CE);
  for (++I; I != E; ++I)
    if (const StructType *STy = dyn_cast<StructType>(*I)) {
      ConstantInt *CU = cast<ConstantInt>(I.getOperand());
      assert(CU->getZExtValue() < STy->getNumElements() &&
             "Struct index out of range!");
      unsigned El = (unsigned)CU->getZExtValue();
      if (ConstantStruct *CS = dyn_cast<ConstantStruct>(C)) {
        C = CS->getOperand(El);
      } else if (isa<ConstantAggregateZero>(C)) {
        C = Constant::getNullValue(STy->getElementType(El));
      } else if (isa<UndefValue>(C)) {
        C = UndefValue::get(STy->getElementType(El));
      } else {
        return 0;
      }
    } else if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand())) {
      if (const ArrayType *ATy = dyn_cast<ArrayType>(*I)) {
        if (CI->getZExtValue() >= ATy->getNumElements())
         return 0;
        if (ConstantArray *CA = dyn_cast<ConstantArray>(C))
          C = CA->getOperand(CI->getZExtValue());
        else if (isa<ConstantAggregateZero>(C))
          C = Constant::getNullValue(ATy->getElementType());
        else if (isa<UndefValue>(C))
          C = UndefValue::get(ATy->getElementType());
        else
          return 0;
      } else if (const VectorType *PTy = dyn_cast<VectorType>(*I)) {
        if (CI->getZExtValue() >= PTy->getNumElements())
          return 0;
        if (ConstantVector *CP = dyn_cast<ConstantVector>(C))
          C = CP->getOperand(CI->getZExtValue());
        else if (isa<ConstantAggregateZero>(C))
          C = Constant::getNullValue(PTy->getElementType());
        else if (isa<UndefValue>(C))
          C = UndefValue::get(PTy->getElementType());
        else
          return 0;
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  return C;
}


//===----------------------------------------------------------------------===//
//  Constant Folding for Calls
//

/// canConstantFoldCallTo - Return true if its even possible to fold a call to
/// the specified function.
bool
llvm::canConstantFoldCallTo(Function *F) {
  switch (F->getIntrinsicID()) {
  case Intrinsic::sqrt:
  case Intrinsic::powi:
  case Intrinsic::bswap:
  case Intrinsic::ctpop:
  case Intrinsic::ctlz:
  case Intrinsic::cttz:
    return true;
  default: break;
  }

  const ValueName *NameVal = F->getValueName();
  if (NameVal == 0) return false;
  const char *Str = NameVal->getKeyData();
  unsigned Len = NameVal->getKeyLength();
  
  // In these cases, the check of the length is required.  We don't want to
  // return true for a name like "cos\0blah" which strcmp would return equal to
  // "cos", but has length 8.
  switch (Str[0]) {
  default: return false;
  case 'a':
    if (Len == 4)
      return !strcmp(Str, "acos") || !strcmp(Str, "asin") ||
             !strcmp(Str, "atan");
    else if (Len == 5)
      return !strcmp(Str, "atan2");
    return false;
  case 'c':
    if (Len == 3)
      return !strcmp(Str, "cos");
    else if (Len == 4)
      return !strcmp(Str, "ceil") || !strcmp(Str, "cosf") ||
             !strcmp(Str, "cosh");
    return false;
  case 'e':
    if (Len == 3)
      return !strcmp(Str, "exp");
    return false;
  case 'f':
    if (Len == 4)
      return !strcmp(Str, "fabs") || !strcmp(Str, "fmod");
    else if (Len == 5)
      return !strcmp(Str, "floor");
    return false;
    break;
  case 'l':
    if (Len == 3 && !strcmp(Str, "log"))
      return true;
    if (Len == 5 && !strcmp(Str, "log10"))
      return true;
    return false;
  case 'p':
    if (Len == 3 && !strcmp(Str, "pow"))
      return true;
    return false;
  case 's':
    if (Len == 3)
      return !strcmp(Str, "sin");
    if (Len == 4)
      return !strcmp(Str, "sinh") || !strcmp(Str, "sqrt");
    if (Len == 5)
      return !strcmp(Str, "sqrtf");
    return false;
  case 't':
    if (Len == 3 && !strcmp(Str, "tan"))
      return true;
    else if (Len == 4 && !strcmp(Str, "tanh"))
      return true;
    return false;
  }
}

static Constant *ConstantFoldFP(double (*NativeFP)(double), double V, 
                                const Type *Ty) {
  errno = 0;
  V = NativeFP(V);
  if (errno == 0) {
    if (Ty==Type::FloatTy)
      return ConstantFP::get(Ty, APFloat((float)V));
    else if (Ty==Type::DoubleTy)
      return ConstantFP::get(Ty, APFloat(V));
    else
      assert(0);
  }
  errno = 0;
  return 0;
}

static Constant *ConstantFoldBinaryFP(double (*NativeFP)(double, double),
                                      double V, double W,
                                      const Type *Ty) {
  errno = 0;
  V = NativeFP(V, W);
  if (errno == 0) {
    if (Ty==Type::FloatTy)
      return ConstantFP::get(Ty, APFloat((float)V));
    else if (Ty==Type::DoubleTy)
      return ConstantFP::get(Ty, APFloat(V));
    else
      assert(0);
  }
  errno = 0;
  return 0;
}

/// ConstantFoldCall - Attempt to constant fold a call to the specified function
/// with the specified arguments, returning null if unsuccessful.

Constant *
llvm::ConstantFoldCall(Function *F, Constant** Operands, unsigned NumOperands) {
  const ValueName *NameVal = F->getValueName();
  if (NameVal == 0) return 0;
  const char *Str = NameVal->getKeyData();
  unsigned Len = NameVal->getKeyLength();
  
  const Type *Ty = F->getReturnType();
  if (NumOperands == 1) {
    if (ConstantFP *Op = dyn_cast<ConstantFP>(Operands[0])) {
      if (Ty!=Type::FloatTy && Ty!=Type::DoubleTy)
        return 0;
      /// Currently APFloat versions of these functions do not exist, so we use
      /// the host native double versions.  Float versions are not called
      /// directly but for all these it is true (float)(f((double)arg)) ==
      /// f(arg).  Long double not supported yet.
      double V = Ty==Type::FloatTy ? (double)Op->getValueAPF().convertToFloat():
                                     Op->getValueAPF().convertToDouble();
      switch (Str[0]) {
      case 'a':
        if (Len == 4 && !strcmp(Str, "acos"))
          return ConstantFoldFP(acos, V, Ty);
        else if (Len == 4 && !strcmp(Str, "asin"))
          return ConstantFoldFP(asin, V, Ty);
        else if (Len == 4 && !strcmp(Str, "atan"))
          return ConstantFoldFP(atan, V, Ty);
        break;
      case 'c':
        if (Len == 4 && !strcmp(Str, "ceil"))
          return ConstantFoldFP(ceil, V, Ty);
        else if (Len == 3 && !strcmp(Str, "cos"))
          return ConstantFoldFP(cos, V, Ty);
        else if (Len == 4 && !strcmp(Str, "cosh"))
          return ConstantFoldFP(cosh, V, Ty);
        break;
      case 'e':
        if (Len == 3 && !strcmp(Str, "exp"))
          return ConstantFoldFP(exp, V, Ty);
        break;
      case 'f':
        if (Len == 4 && !strcmp(Str, "fabs"))
          return ConstantFoldFP(fabs, V, Ty);
        else if (Len == 5 && !strcmp(Str, "floor"))
          return ConstantFoldFP(floor, V, Ty);
        break;
      case 'l':
        if (Len == 3 && !strcmp(Str, "log") && V > 0)
          return ConstantFoldFP(log, V, Ty);
        else if (Len == 5 && !strcmp(Str, "log10") && V > 0)
          return ConstantFoldFP(log10, V, Ty);
        else if (!strcmp(Str, "llvm.sqrt.f32") ||
                 !strcmp(Str, "llvm.sqrt.f64")) {
          if (V >= -0.0)
            return ConstantFoldFP(sqrt, V, Ty);
          else // Undefined
            return ConstantFP::get(Ty, Ty==Type::FloatTy ? APFloat(0.0f) :
                                       APFloat(0.0));
        }
        break;
      case 's':
        if (Len == 3 && !strcmp(Str, "sin"))
          return ConstantFoldFP(sin, V, Ty);
        else if (Len == 4 && !strcmp(Str, "sinh"))
          return ConstantFoldFP(sinh, V, Ty);
        else if (Len == 4 && !strcmp(Str, "sqrt") && V >= 0)
          return ConstantFoldFP(sqrt, V, Ty);
        else if (Len == 5 && !strcmp(Str, "sqrtf") && V >= 0)
          return ConstantFoldFP(sqrt, V, Ty);
        break;
      case 't':
        if (Len == 3 && !strcmp(Str, "tan"))
          return ConstantFoldFP(tan, V, Ty);
        else if (Len == 4 && !strcmp(Str, "tanh"))
          return ConstantFoldFP(tanh, V, Ty);
        break;
      default:
        break;
      }
    } else if (ConstantInt *Op = dyn_cast<ConstantInt>(Operands[0])) {
      if (Len > 11 && !memcmp(Str, "llvm.bswap", 10))
        return ConstantInt::get(Op->getValue().byteSwap());
      else if (Len > 11 && !memcmp(Str, "llvm.ctpop", 10))
        return ConstantInt::get(Ty, Op->getValue().countPopulation());
      else if (Len > 10 && !memcmp(Str, "llvm.cttz", 9))
        return ConstantInt::get(Ty, Op->getValue().countTrailingZeros());
      else if (Len > 10 && !memcmp(Str, "llvm.ctlz", 9))
        return ConstantInt::get(Ty, Op->getValue().countLeadingZeros());
    }
  } else if (NumOperands == 2) {
    if (ConstantFP *Op1 = dyn_cast<ConstantFP>(Operands[0])) {
      if (Ty!=Type::FloatTy && Ty!=Type::DoubleTy)
        return 0;
      double Op1V = Ty==Type::FloatTy ? 
                      (double)Op1->getValueAPF().convertToFloat():
                      Op1->getValueAPF().convertToDouble();
      if (ConstantFP *Op2 = dyn_cast<ConstantFP>(Operands[1])) {
        double Op2V = Ty==Type::FloatTy ? 
                      (double)Op2->getValueAPF().convertToFloat():
                      Op2->getValueAPF().convertToDouble();

        if (Len == 3 && !strcmp(Str, "pow")) {
          return ConstantFoldBinaryFP(pow, Op1V, Op2V, Ty);
        } else if (Len == 4 && !strcmp(Str, "fmod")) {
          return ConstantFoldBinaryFP(fmod, Op1V, Op2V, Ty);
        } else if (Len == 5 && !strcmp(Str, "atan2")) {
          return ConstantFoldBinaryFP(atan2, Op1V, Op2V, Ty);
        }
      } else if (ConstantInt *Op2C = dyn_cast<ConstantInt>(Operands[1])) {
        if (!strcmp(Str, "llvm.powi.f32")) {
          return ConstantFP::get(Ty, APFloat((float)std::pow((float)Op1V,
                                              (int)Op2C->getZExtValue())));
        } else if (!strcmp(Str, "llvm.powi.f64")) {
          return ConstantFP::get(Ty, APFloat((double)std::pow((double)Op1V,
                                              (int)Op2C->getZExtValue())));
        }
      }
    }
  }
  return 0;
}

