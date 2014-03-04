//===- InstructionCombining.cpp - Combine multiple instructions -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// InstructionCombining - Combine instructions to form fewer, simple
// instructions.  This pass does not modify the CFG.  This pass is where
// algebraic simplification happens.
//
// This pass combines things like:
//    %Y = add i32 %X, 1
//    %Z = add i32 %Y, 1
// into:
//    %Z = add i32 %X, 2
//
// This is a simple worklist driven algorithm.
//
// This pass guarantees that the following canonicalizations are performed on
// the program:
//    1. If a binary operator has a constant operand, it is moved to the RHS
//    2. Bitwise operators with constant operands are always grouped so that
//       shifts are performed first, then or's, then and's, then xor's.
//    3. Compare instructions are converted from <,>,<=,>= to ==,!= if possible
//    4. All cmp instructions on boolean values are replaced with logical ops
//    5. add X, X is represented as (X*2) => (X << 1)
//    6. Multiplies with a power-of-two constant argument are transformed into
//       shifts.
//   ... etc.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "instcombine"
#include "llvm/Transforms/Scalar.h"
#include "InstCombine.h"
#include "llvm-c/Initialization.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/PatternMatch.h"
#include "llvm/Support/ValueHandle.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/Local.h"
#include <algorithm>
#include <climits>
using namespace llvm;
using namespace llvm::PatternMatch;

STATISTIC(NumCombined , "Number of insts combined");
STATISTIC(NumConstProp, "Number of constant folds");
STATISTIC(NumDeadInst , "Number of dead inst eliminated");
STATISTIC(NumSunkInst , "Number of instructions sunk");
STATISTIC(NumExpand,    "Number of expansions");
STATISTIC(NumFactor   , "Number of factorizations");
STATISTIC(NumReassoc  , "Number of reassociations");

static cl::opt<bool> UnsafeFPShrink("enable-double-float-shrink", cl::Hidden,
                                   cl::init(false),
                                   cl::desc("Enable unsafe double to float "
                                            "shrinking for math lib calls"));

// Initialization Routines
void llvm::initializeInstCombine(PassRegistry &Registry) {
  initializeInstCombinerPass(Registry);
}

void LLVMInitializeInstCombine(LLVMPassRegistryRef R) {
  initializeInstCombine(*unwrap(R));
}

char InstCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(InstCombiner, "instcombine",
                "Combine redundant instructions", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfo)
INITIALIZE_PASS_END(InstCombiner, "instcombine",
                "Combine redundant instructions", false, false)

void InstCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<TargetLibraryInfo>();
}


Value *InstCombiner::EmitGEPOffset(User *GEP) {
  return llvm::EmitGEPOffset(Builder, *getDataLayout(), GEP);
}

/// ShouldChangeType - Return true if it is desirable to convert a computation
/// from 'From' to 'To'.  We don't want to convert from a legal to an illegal
/// type for example, or from a smaller to a larger illegal type.
bool InstCombiner::ShouldChangeType(Type *From, Type *To) const {
  assert(From->isIntegerTy() && To->isIntegerTy());

  // If we don't have DL, we don't know if the source/dest are legal.
  if (!DL) return false;

  unsigned FromWidth = From->getPrimitiveSizeInBits();
  unsigned ToWidth = To->getPrimitiveSizeInBits();
  bool FromLegal = DL->isLegalInteger(FromWidth);
  bool ToLegal = DL->isLegalInteger(ToWidth);

  // If this is a legal integer from type, and the result would be an illegal
  // type, don't do the transformation.
  if (FromLegal && !ToLegal)
    return false;

  // Otherwise, if both are illegal, do not increase the size of the result. We
  // do allow things like i160 -> i64, but not i64 -> i160.
  if (!FromLegal && !ToLegal && ToWidth > FromWidth)
    return false;

  return true;
}

// Return true, if No Signed Wrap should be maintained for I.
// The No Signed Wrap flag can be kept if the operation "B (I.getOpcode) C",
// where both B and C should be ConstantInts, results in a constant that does
// not overflow. This function only handles the Add and Sub opcodes. For
// all other opcodes, the function conservatively returns false.
static bool MaintainNoSignedWrap(BinaryOperator &I, Value *B, Value *C) {
  OverflowingBinaryOperator *OBO = dyn_cast<OverflowingBinaryOperator>(&I);
  if (!OBO || !OBO->hasNoSignedWrap()) {
    return false;
  }

  // We reason about Add and Sub Only.
  Instruction::BinaryOps Opcode = I.getOpcode();
  if (Opcode != Instruction::Add &&
      Opcode != Instruction::Sub) {
    return false;
  }

  ConstantInt *CB = dyn_cast<ConstantInt>(B);
  ConstantInt *CC = dyn_cast<ConstantInt>(C);

  if (!CB || !CC) {
    return false;
  }

  const APInt &BVal = CB->getValue();
  const APInt &CVal = CC->getValue();
  bool Overflow = false;

  if (Opcode == Instruction::Add) {
    BVal.sadd_ov(CVal, Overflow);
  } else {
    BVal.ssub_ov(CVal, Overflow);
  }

  return !Overflow;
}

/// Conservatively clears subclassOptionalData after a reassociation or
/// commutation. We preserve fast-math flags when applicable as they can be
/// preserved.
static void ClearSubclassDataAfterReassociation(BinaryOperator &I) {
  FPMathOperator *FPMO = dyn_cast<FPMathOperator>(&I);
  if (!FPMO) {
    I.clearSubclassOptionalData();
    return;
  }

  FastMathFlags FMF = I.getFastMathFlags();
  I.clearSubclassOptionalData();
  I.setFastMathFlags(FMF);
}

/// SimplifyAssociativeOrCommutative - This performs a few simplifications for
/// operators which are associative or commutative:
//
//  Commutative operators:
//
//  1. Order operands such that they are listed from right (least complex) to
//     left (most complex).  This puts constants before unary operators before
//     binary operators.
//
//  Associative operators:
//
//  2. Transform: "(A op B) op C" ==> "A op (B op C)" if "B op C" simplifies.
//  3. Transform: "A op (B op C)" ==> "(A op B) op C" if "A op B" simplifies.
//
//  Associative and commutative operators:
//
//  4. Transform: "(A op B) op C" ==> "(C op A) op B" if "C op A" simplifies.
//  5. Transform: "A op (B op C)" ==> "B op (C op A)" if "C op A" simplifies.
//  6. Transform: "(A op C1) op (B op C2)" ==> "(A op B) op (C1 op C2)"
//     if C1 and C2 are constants.
//
bool InstCombiner::SimplifyAssociativeOrCommutative(BinaryOperator &I) {
  Instruction::BinaryOps Opcode = I.getOpcode();
  bool Changed = false;

  do {
    // Order operands such that they are listed from right (least complex) to
    // left (most complex).  This puts constants before unary operators before
    // binary operators.
    if (I.isCommutative() && getComplexity(I.getOperand(0)) <
        getComplexity(I.getOperand(1)))
      Changed = !I.swapOperands();

    BinaryOperator *Op0 = dyn_cast<BinaryOperator>(I.getOperand(0));
    BinaryOperator *Op1 = dyn_cast<BinaryOperator>(I.getOperand(1));

    if (I.isAssociative()) {
      // Transform: "(A op B) op C" ==> "A op (B op C)" if "B op C" simplifies.
      if (Op0 && Op0->getOpcode() == Opcode) {
        Value *A = Op0->getOperand(0);
        Value *B = Op0->getOperand(1);
        Value *C = I.getOperand(1);

        // Does "B op C" simplify?
        if (Value *V = SimplifyBinOp(Opcode, B, C, DL)) {
          // It simplifies to V.  Form "A op V".
          I.setOperand(0, A);
          I.setOperand(1, V);
          // Conservatively clear the optional flags, since they may not be
          // preserved by the reassociation.
          if (MaintainNoSignedWrap(I, B, C) &&
              (!Op0 || (isa<BinaryOperator>(Op0) && Op0->hasNoSignedWrap()))) {
            // Note: this is only valid because SimplifyBinOp doesn't look at
            // the operands to Op0.
            I.clearSubclassOptionalData();
            I.setHasNoSignedWrap(true);
          } else {
            ClearSubclassDataAfterReassociation(I);
          }

          Changed = true;
          ++NumReassoc;
          continue;
        }
      }

      // Transform: "A op (B op C)" ==> "(A op B) op C" if "A op B" simplifies.
      if (Op1 && Op1->getOpcode() == Opcode) {
        Value *A = I.getOperand(0);
        Value *B = Op1->getOperand(0);
        Value *C = Op1->getOperand(1);

        // Does "A op B" simplify?
        if (Value *V = SimplifyBinOp(Opcode, A, B, DL)) {
          // It simplifies to V.  Form "V op C".
          I.setOperand(0, V);
          I.setOperand(1, C);
          // Conservatively clear the optional flags, since they may not be
          // preserved by the reassociation.
          ClearSubclassDataAfterReassociation(I);
          Changed = true;
          ++NumReassoc;
          continue;
        }
      }
    }

    if (I.isAssociative() && I.isCommutative()) {
      // Transform: "(A op B) op C" ==> "(C op A) op B" if "C op A" simplifies.
      if (Op0 && Op0->getOpcode() == Opcode) {
        Value *A = Op0->getOperand(0);
        Value *B = Op0->getOperand(1);
        Value *C = I.getOperand(1);

        // Does "C op A" simplify?
        if (Value *V = SimplifyBinOp(Opcode, C, A, DL)) {
          // It simplifies to V.  Form "V op B".
          I.setOperand(0, V);
          I.setOperand(1, B);
          // Conservatively clear the optional flags, since they may not be
          // preserved by the reassociation.
          ClearSubclassDataAfterReassociation(I);
          Changed = true;
          ++NumReassoc;
          continue;
        }
      }

      // Transform: "A op (B op C)" ==> "B op (C op A)" if "C op A" simplifies.
      if (Op1 && Op1->getOpcode() == Opcode) {
        Value *A = I.getOperand(0);
        Value *B = Op1->getOperand(0);
        Value *C = Op1->getOperand(1);

        // Does "C op A" simplify?
        if (Value *V = SimplifyBinOp(Opcode, C, A, DL)) {
          // It simplifies to V.  Form "B op V".
          I.setOperand(0, B);
          I.setOperand(1, V);
          // Conservatively clear the optional flags, since they may not be
          // preserved by the reassociation.
          ClearSubclassDataAfterReassociation(I);
          Changed = true;
          ++NumReassoc;
          continue;
        }
      }

      // Transform: "(A op C1) op (B op C2)" ==> "(A op B) op (C1 op C2)"
      // if C1 and C2 are constants.
      if (Op0 && Op1 &&
          Op0->getOpcode() == Opcode && Op1->getOpcode() == Opcode &&
          isa<Constant>(Op0->getOperand(1)) &&
          isa<Constant>(Op1->getOperand(1)) &&
          Op0->hasOneUse() && Op1->hasOneUse()) {
        Value *A = Op0->getOperand(0);
        Constant *C1 = cast<Constant>(Op0->getOperand(1));
        Value *B = Op1->getOperand(0);
        Constant *C2 = cast<Constant>(Op1->getOperand(1));

        Constant *Folded = ConstantExpr::get(Opcode, C1, C2);
        BinaryOperator *New = BinaryOperator::Create(Opcode, A, B);
        if (isa<FPMathOperator>(New)) {
          FastMathFlags Flags = I.getFastMathFlags();
          Flags &= Op0->getFastMathFlags();
          Flags &= Op1->getFastMathFlags();
          New->setFastMathFlags(Flags);
        }
        InsertNewInstWith(New, I);
        New->takeName(Op1);
        I.setOperand(0, New);
        I.setOperand(1, Folded);
        // Conservatively clear the optional flags, since they may not be
        // preserved by the reassociation.
        ClearSubclassDataAfterReassociation(I);

        Changed = true;
        continue;
      }
    }

    // No further simplifications.
    return Changed;
  } while (1);
}

/// LeftDistributesOverRight - Whether "X LOp (Y ROp Z)" is always equal to
/// "(X LOp Y) ROp (X LOp Z)".
static bool LeftDistributesOverRight(Instruction::BinaryOps LOp,
                                     Instruction::BinaryOps ROp) {
  switch (LOp) {
  default:
    return false;

  case Instruction::And:
    // And distributes over Or and Xor.
    switch (ROp) {
    default:
      return false;
    case Instruction::Or:
    case Instruction::Xor:
      return true;
    }

  case Instruction::Mul:
    // Multiplication distributes over addition and subtraction.
    switch (ROp) {
    default:
      return false;
    case Instruction::Add:
    case Instruction::Sub:
      return true;
    }

  case Instruction::Or:
    // Or distributes over And.
    switch (ROp) {
    default:
      return false;
    case Instruction::And:
      return true;
    }
  }
}

/// RightDistributesOverLeft - Whether "(X LOp Y) ROp Z" is always equal to
/// "(X ROp Z) LOp (Y ROp Z)".
static bool RightDistributesOverLeft(Instruction::BinaryOps LOp,
                                     Instruction::BinaryOps ROp) {
  if (Instruction::isCommutative(ROp))
    return LeftDistributesOverRight(ROp, LOp);
  // TODO: It would be nice to handle division, aka "(X + Y)/Z = X/Z + Y/Z",
  // but this requires knowing that the addition does not overflow and other
  // such subtleties.
  return false;
}

/// SimplifyUsingDistributiveLaws - This tries to simplify binary operations
/// which some other binary operation distributes over either by factorizing
/// out common terms (eg "(A*B)+(A*C)" -> "A*(B+C)") or expanding out if this
/// results in simplifications (eg: "A & (B | C) -> (A&B) | (A&C)" if this is
/// a win).  Returns the simplified value, or null if it didn't simplify.
Value *InstCombiner::SimplifyUsingDistributiveLaws(BinaryOperator &I) {
  Value *LHS = I.getOperand(0), *RHS = I.getOperand(1);
  BinaryOperator *Op0 = dyn_cast<BinaryOperator>(LHS);
  BinaryOperator *Op1 = dyn_cast<BinaryOperator>(RHS);
  Instruction::BinaryOps TopLevelOpcode = I.getOpcode(); // op

  // Factorization.
  if (Op0 && Op1 && Op0->getOpcode() == Op1->getOpcode()) {
    // The instruction has the form "(A op' B) op (C op' D)".  Try to factorize
    // a common term.
    Value *A = Op0->getOperand(0), *B = Op0->getOperand(1);
    Value *C = Op1->getOperand(0), *D = Op1->getOperand(1);
    Instruction::BinaryOps InnerOpcode = Op0->getOpcode(); // op'

    // Does "X op' Y" always equal "Y op' X"?
    bool InnerCommutative = Instruction::isCommutative(InnerOpcode);

    // Does "X op' (Y op Z)" always equal "(X op' Y) op (X op' Z)"?
    if (LeftDistributesOverRight(InnerOpcode, TopLevelOpcode))
      // Does the instruction have the form "(A op' B) op (A op' D)" or, in the
      // commutative case, "(A op' B) op (C op' A)"?
      if (A == C || (InnerCommutative && A == D)) {
        if (A != C)
          std::swap(C, D);
        // Consider forming "A op' (B op D)".
        // If "B op D" simplifies then it can be formed with no cost.
        Value *V = SimplifyBinOp(TopLevelOpcode, B, D, DL);
        // If "B op D" doesn't simplify then only go on if both of the existing
        // operations "A op' B" and "C op' D" will be zapped as no longer used.
        if (!V && Op0->hasOneUse() && Op1->hasOneUse())
          V = Builder->CreateBinOp(TopLevelOpcode, B, D, Op1->getName());
        if (V) {
          ++NumFactor;
          V = Builder->CreateBinOp(InnerOpcode, A, V);
          V->takeName(&I);
          return V;
        }
      }

    // Does "(X op Y) op' Z" always equal "(X op' Z) op (Y op' Z)"?
    if (RightDistributesOverLeft(TopLevelOpcode, InnerOpcode))
      // Does the instruction have the form "(A op' B) op (C op' B)" or, in the
      // commutative case, "(A op' B) op (B op' D)"?
      if (B == D || (InnerCommutative && B == C)) {
        if (B != D)
          std::swap(C, D);
        // Consider forming "(A op C) op' B".
        // If "A op C" simplifies then it can be formed with no cost.
        Value *V = SimplifyBinOp(TopLevelOpcode, A, C, DL);
        // If "A op C" doesn't simplify then only go on if both of the existing
        // operations "A op' B" and "C op' D" will be zapped as no longer used.
        if (!V && Op0->hasOneUse() && Op1->hasOneUse())
          V = Builder->CreateBinOp(TopLevelOpcode, A, C, Op0->getName());
        if (V) {
          ++NumFactor;
          V = Builder->CreateBinOp(InnerOpcode, V, B);
          V->takeName(&I);
          return V;
        }
      }
  }

  // Expansion.
  if (Op0 && RightDistributesOverLeft(Op0->getOpcode(), TopLevelOpcode)) {
    // The instruction has the form "(A op' B) op C".  See if expanding it out
    // to "(A op C) op' (B op C)" results in simplifications.
    Value *A = Op0->getOperand(0), *B = Op0->getOperand(1), *C = RHS;
    Instruction::BinaryOps InnerOpcode = Op0->getOpcode(); // op'

    // Do "A op C" and "B op C" both simplify?
    if (Value *L = SimplifyBinOp(TopLevelOpcode, A, C, DL))
      if (Value *R = SimplifyBinOp(TopLevelOpcode, B, C, DL)) {
        // They do! Return "L op' R".
        ++NumExpand;
        // If "L op' R" equals "A op' B" then "L op' R" is just the LHS.
        if ((L == A && R == B) ||
            (Instruction::isCommutative(InnerOpcode) && L == B && R == A))
          return Op0;
        // Otherwise return "L op' R" if it simplifies.
        if (Value *V = SimplifyBinOp(InnerOpcode, L, R, DL))
          return V;
        // Otherwise, create a new instruction.
        C = Builder->CreateBinOp(InnerOpcode, L, R);
        C->takeName(&I);
        return C;
      }
  }

  if (Op1 && LeftDistributesOverRight(TopLevelOpcode, Op1->getOpcode())) {
    // The instruction has the form "A op (B op' C)".  See if expanding it out
    // to "(A op B) op' (A op C)" results in simplifications.
    Value *A = LHS, *B = Op1->getOperand(0), *C = Op1->getOperand(1);
    Instruction::BinaryOps InnerOpcode = Op1->getOpcode(); // op'

    // Do "A op B" and "A op C" both simplify?
    if (Value *L = SimplifyBinOp(TopLevelOpcode, A, B, DL))
      if (Value *R = SimplifyBinOp(TopLevelOpcode, A, C, DL)) {
        // They do! Return "L op' R".
        ++NumExpand;
        // If "L op' R" equals "B op' C" then "L op' R" is just the RHS.
        if ((L == B && R == C) ||
            (Instruction::isCommutative(InnerOpcode) && L == C && R == B))
          return Op1;
        // Otherwise return "L op' R" if it simplifies.
        if (Value *V = SimplifyBinOp(InnerOpcode, L, R, DL))
          return V;
        // Otherwise, create a new instruction.
        A = Builder->CreateBinOp(InnerOpcode, L, R);
        A->takeName(&I);
        return A;
      }
  }

  return 0;
}

// dyn_castNegVal - Given a 'sub' instruction, return the RHS of the instruction
// if the LHS is a constant zero (which is the 'negate' form).
//
Value *InstCombiner::dyn_castNegVal(Value *V) const {
  if (BinaryOperator::isNeg(V))
    return BinaryOperator::getNegArgument(V);

  // Constants can be considered to be negated values if they can be folded.
  if (ConstantInt *C = dyn_cast<ConstantInt>(V))
    return ConstantExpr::getNeg(C);

  if (ConstantDataVector *C = dyn_cast<ConstantDataVector>(V))
    if (C->getType()->getElementType()->isIntegerTy())
      return ConstantExpr::getNeg(C);

  return 0;
}

// dyn_castFNegVal - Given a 'fsub' instruction, return the RHS of the
// instruction if the LHS is a constant negative zero (which is the 'negate'
// form).
//
Value *InstCombiner::dyn_castFNegVal(Value *V, bool IgnoreZeroSign) const {
  if (BinaryOperator::isFNeg(V, IgnoreZeroSign))
    return BinaryOperator::getFNegArgument(V);

  // Constants can be considered to be negated values if they can be folded.
  if (ConstantFP *C = dyn_cast<ConstantFP>(V))
    return ConstantExpr::getFNeg(C);

  if (ConstantDataVector *C = dyn_cast<ConstantDataVector>(V))
    if (C->getType()->getElementType()->isFloatingPointTy())
      return ConstantExpr::getFNeg(C);

  return 0;
}

static Value *FoldOperationIntoSelectOperand(Instruction &I, Value *SO,
                                             InstCombiner *IC) {
  if (CastInst *CI = dyn_cast<CastInst>(&I)) {
    return IC->Builder->CreateCast(CI->getOpcode(), SO, I.getType());
  }

  // Figure out if the constant is the left or the right argument.
  bool ConstIsRHS = isa<Constant>(I.getOperand(1));
  Constant *ConstOperand = cast<Constant>(I.getOperand(ConstIsRHS));

  if (Constant *SOC = dyn_cast<Constant>(SO)) {
    if (ConstIsRHS)
      return ConstantExpr::get(I.getOpcode(), SOC, ConstOperand);
    return ConstantExpr::get(I.getOpcode(), ConstOperand, SOC);
  }

  Value *Op0 = SO, *Op1 = ConstOperand;
  if (!ConstIsRHS)
    std::swap(Op0, Op1);

  if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&I)) {
    Value *RI = IC->Builder->CreateBinOp(BO->getOpcode(), Op0, Op1,
                                    SO->getName()+".op");
    Instruction *FPInst = dyn_cast<Instruction>(RI);
    if (FPInst && isa<FPMathOperator>(FPInst))
      FPInst->copyFastMathFlags(BO);
    return RI;
  }
  if (ICmpInst *CI = dyn_cast<ICmpInst>(&I))
    return IC->Builder->CreateICmp(CI->getPredicate(), Op0, Op1,
                                   SO->getName()+".cmp");
  if (FCmpInst *CI = dyn_cast<FCmpInst>(&I))
    return IC->Builder->CreateICmp(CI->getPredicate(), Op0, Op1,
                                   SO->getName()+".cmp");
  llvm_unreachable("Unknown binary instruction type!");
}

// FoldOpIntoSelect - Given an instruction with a select as one operand and a
// constant as the other operand, try to fold the binary operator into the
// select arguments.  This also works for Cast instructions, which obviously do
// not have a second operand.
Instruction *InstCombiner::FoldOpIntoSelect(Instruction &Op, SelectInst *SI) {
  // Don't modify shared select instructions
  if (!SI->hasOneUse()) return 0;
  Value *TV = SI->getOperand(1);
  Value *FV = SI->getOperand(2);

  if (isa<Constant>(TV) || isa<Constant>(FV)) {
    // Bool selects with constant operands can be folded to logical ops.
    if (SI->getType()->isIntegerTy(1)) return 0;

    // If it's a bitcast involving vectors, make sure it has the same number of
    // elements on both sides.
    if (BitCastInst *BC = dyn_cast<BitCastInst>(&Op)) {
      VectorType *DestTy = dyn_cast<VectorType>(BC->getDestTy());
      VectorType *SrcTy = dyn_cast<VectorType>(BC->getSrcTy());

      // Verify that either both or neither are vectors.
      if ((SrcTy == NULL) != (DestTy == NULL)) return 0;
      // If vectors, verify that they have the same number of elements.
      if (SrcTy && SrcTy->getNumElements() != DestTy->getNumElements())
        return 0;
    }

    Value *SelectTrueVal = FoldOperationIntoSelectOperand(Op, TV, this);
    Value *SelectFalseVal = FoldOperationIntoSelectOperand(Op, FV, this);

    return SelectInst::Create(SI->getCondition(),
                              SelectTrueVal, SelectFalseVal);
  }
  return 0;
}


/// FoldOpIntoPhi - Given a binary operator, cast instruction, or select which
/// has a PHI node as operand #0, see if we can fold the instruction into the
/// PHI (which is only possible if all operands to the PHI are constants).
///
Instruction *InstCombiner::FoldOpIntoPhi(Instruction &I) {
  PHINode *PN = cast<PHINode>(I.getOperand(0));
  unsigned NumPHIValues = PN->getNumIncomingValues();
  if (NumPHIValues == 0)
    return 0;

  // We normally only transform phis with a single use.  However, if a PHI has
  // multiple uses and they are all the same operation, we can fold *all* of the
  // uses into the PHI.
  if (!PN->hasOneUse()) {
    // Walk the use list for the instruction, comparing them to I.
    for (Value::use_iterator UI = PN->use_begin(), E = PN->use_end();
         UI != E; ++UI) {
      Instruction *User = cast<Instruction>(*UI);
      if (User != &I && !I.isIdenticalTo(User))
        return 0;
    }
    // Otherwise, we can replace *all* users with the new PHI we form.
  }

  // Check to see if all of the operands of the PHI are simple constants
  // (constantint/constantfp/undef).  If there is one non-constant value,
  // remember the BB it is in.  If there is more than one or if *it* is a PHI,
  // bail out.  We don't do arbitrary constant expressions here because moving
  // their computation can be expensive without a cost model.
  BasicBlock *NonConstBB = 0;
  for (unsigned i = 0; i != NumPHIValues; ++i) {
    Value *InVal = PN->getIncomingValue(i);
    if (isa<Constant>(InVal) && !isa<ConstantExpr>(InVal))
      continue;

    if (isa<PHINode>(InVal)) return 0;  // Itself a phi.
    if (NonConstBB) return 0;  // More than one non-const value.

    NonConstBB = PN->getIncomingBlock(i);

    // If the InVal is an invoke at the end of the pred block, then we can't
    // insert a computation after it without breaking the edge.
    if (InvokeInst *II = dyn_cast<InvokeInst>(InVal))
      if (II->getParent() == NonConstBB)
        return 0;

    // If the incoming non-constant value is in I's block, we will remove one
    // instruction, but insert another equivalent one, leading to infinite
    // instcombine.
    if (NonConstBB == I.getParent())
      return 0;
  }

  // If there is exactly one non-constant value, we can insert a copy of the
  // operation in that block.  However, if this is a critical edge, we would be
  // inserting the computation one some other paths (e.g. inside a loop).  Only
  // do this if the pred block is unconditionally branching into the phi block.
  if (NonConstBB != 0) {
    BranchInst *BI = dyn_cast<BranchInst>(NonConstBB->getTerminator());
    if (!BI || !BI->isUnconditional()) return 0;
  }

  // Okay, we can do the transformation: create the new PHI node.
  PHINode *NewPN = PHINode::Create(I.getType(), PN->getNumIncomingValues());
  InsertNewInstBefore(NewPN, *PN);
  NewPN->takeName(PN);

  // If we are going to have to insert a new computation, do so right before the
  // predecessors terminator.
  if (NonConstBB)
    Builder->SetInsertPoint(NonConstBB->getTerminator());

  // Next, add all of the operands to the PHI.
  if (SelectInst *SI = dyn_cast<SelectInst>(&I)) {
    // We only currently try to fold the condition of a select when it is a phi,
    // not the true/false values.
    Value *TrueV = SI->getTrueValue();
    Value *FalseV = SI->getFalseValue();
    BasicBlock *PhiTransBB = PN->getParent();
    for (unsigned i = 0; i != NumPHIValues; ++i) {
      BasicBlock *ThisBB = PN->getIncomingBlock(i);
      Value *TrueVInPred = TrueV->DoPHITranslation(PhiTransBB, ThisBB);
      Value *FalseVInPred = FalseV->DoPHITranslation(PhiTransBB, ThisBB);
      Value *InV = 0;
      // Beware of ConstantExpr:  it may eventually evaluate to getNullValue,
      // even if currently isNullValue gives false.
      Constant *InC = dyn_cast<Constant>(PN->getIncomingValue(i));
      if (InC && !isa<ConstantExpr>(InC))
        InV = InC->isNullValue() ? FalseVInPred : TrueVInPred;
      else
        InV = Builder->CreateSelect(PN->getIncomingValue(i),
                                    TrueVInPred, FalseVInPred, "phitmp");
      NewPN->addIncoming(InV, ThisBB);
    }
  } else if (CmpInst *CI = dyn_cast<CmpInst>(&I)) {
    Constant *C = cast<Constant>(I.getOperand(1));
    for (unsigned i = 0; i != NumPHIValues; ++i) {
      Value *InV = 0;
      if (Constant *InC = dyn_cast<Constant>(PN->getIncomingValue(i)))
        InV = ConstantExpr::getCompare(CI->getPredicate(), InC, C);
      else if (isa<ICmpInst>(CI))
        InV = Builder->CreateICmp(CI->getPredicate(), PN->getIncomingValue(i),
                                  C, "phitmp");
      else
        InV = Builder->CreateFCmp(CI->getPredicate(), PN->getIncomingValue(i),
                                  C, "phitmp");
      NewPN->addIncoming(InV, PN->getIncomingBlock(i));
    }
  } else if (I.getNumOperands() == 2) {
    Constant *C = cast<Constant>(I.getOperand(1));
    for (unsigned i = 0; i != NumPHIValues; ++i) {
      Value *InV = 0;
      if (Constant *InC = dyn_cast<Constant>(PN->getIncomingValue(i)))
        InV = ConstantExpr::get(I.getOpcode(), InC, C);
      else
        InV = Builder->CreateBinOp(cast<BinaryOperator>(I).getOpcode(),
                                   PN->getIncomingValue(i), C, "phitmp");
      NewPN->addIncoming(InV, PN->getIncomingBlock(i));
    }
  } else {
    CastInst *CI = cast<CastInst>(&I);
    Type *RetTy = CI->getType();
    for (unsigned i = 0; i != NumPHIValues; ++i) {
      Value *InV;
      if (Constant *InC = dyn_cast<Constant>(PN->getIncomingValue(i)))
        InV = ConstantExpr::getCast(CI->getOpcode(), InC, RetTy);
      else
        InV = Builder->CreateCast(CI->getOpcode(),
                                PN->getIncomingValue(i), I.getType(), "phitmp");
      NewPN->addIncoming(InV, PN->getIncomingBlock(i));
    }
  }

  for (Value::use_iterator UI = PN->use_begin(), E = PN->use_end();
       UI != E; ) {
    Instruction *User = cast<Instruction>(*UI++);
    if (User == &I) continue;
    ReplaceInstUsesWith(*User, NewPN);
    EraseInstFromFunction(*User);
  }
  return ReplaceInstUsesWith(I, NewPN);
}

/// FindElementAtOffset - Given a pointer type and a constant offset, determine
/// whether or not there is a sequence of GEP indices into the pointed type that
/// will land us at the specified offset.  If so, fill them into NewIndices and
/// return the resultant element type, otherwise return null.
Type *InstCombiner::FindElementAtOffset(Type *PtrTy, int64_t Offset,
                                        SmallVectorImpl<Value*> &NewIndices) {
  assert(PtrTy->isPtrOrPtrVectorTy());

  if (!DL)
    return 0;

  Type *Ty = PtrTy->getPointerElementType();
  if (!Ty->isSized())
    return 0;

  // Start with the index over the outer type.  Note that the type size
  // might be zero (even if the offset isn't zero) if the indexed type
  // is something like [0 x {int, int}]
  Type *IntPtrTy = DL->getIntPtrType(PtrTy);
  int64_t FirstIdx = 0;
  if (int64_t TySize = DL->getTypeAllocSize(Ty)) {
    FirstIdx = Offset/TySize;
    Offset -= FirstIdx*TySize;

    // Handle hosts where % returns negative instead of values [0..TySize).
    if (Offset < 0) {
      --FirstIdx;
      Offset += TySize;
      assert(Offset >= 0);
    }
    assert((uint64_t)Offset < (uint64_t)TySize && "Out of range offset");
  }

  NewIndices.push_back(ConstantInt::get(IntPtrTy, FirstIdx));

  // Index into the types.  If we fail, set OrigBase to null.
  while (Offset) {
    // Indexing into tail padding between struct/array elements.
    if (uint64_t(Offset*8) >= DL->getTypeSizeInBits(Ty))
      return 0;

    if (StructType *STy = dyn_cast<StructType>(Ty)) {
      const StructLayout *SL = DL->getStructLayout(STy);
      assert(Offset < (int64_t)SL->getSizeInBytes() &&
             "Offset must stay within the indexed type");

      unsigned Elt = SL->getElementContainingOffset(Offset);
      NewIndices.push_back(ConstantInt::get(Type::getInt32Ty(Ty->getContext()),
                                            Elt));

      Offset -= SL->getElementOffset(Elt);
      Ty = STy->getElementType(Elt);
    } else if (ArrayType *AT = dyn_cast<ArrayType>(Ty)) {
      uint64_t EltSize = DL->getTypeAllocSize(AT->getElementType());
      assert(EltSize && "Cannot index into a zero-sized array");
      NewIndices.push_back(ConstantInt::get(IntPtrTy,Offset/EltSize));
      Offset %= EltSize;
      Ty = AT->getElementType();
    } else {
      // Otherwise, we can't index into the middle of this atomic type, bail.
      return 0;
    }
  }

  return Ty;
}

static bool shouldMergeGEPs(GEPOperator &GEP, GEPOperator &Src) {
  // If this GEP has only 0 indices, it is the same pointer as
  // Src. If Src is not a trivial GEP too, don't combine
  // the indices.
  if (GEP.hasAllZeroIndices() && !Src.hasAllZeroIndices() &&
      !Src.hasOneUse())
    return false;
  return true;
}

/// Descale - Return a value X such that Val = X * Scale, or null if none.  If
/// the multiplication is known not to overflow then NoSignedWrap is set.
Value *InstCombiner::Descale(Value *Val, APInt Scale, bool &NoSignedWrap) {
  assert(isa<IntegerType>(Val->getType()) && "Can only descale integers!");
  assert(cast<IntegerType>(Val->getType())->getBitWidth() ==
         Scale.getBitWidth() && "Scale not compatible with value!");

  // If Val is zero or Scale is one then Val = Val * Scale.
  if (match(Val, m_Zero()) || Scale == 1) {
    NoSignedWrap = true;
    return Val;
  }

  // If Scale is zero then it does not divide Val.
  if (Scale.isMinValue())
    return 0;

  // Look through chains of multiplications, searching for a constant that is
  // divisible by Scale.  For example, descaling X*(Y*(Z*4)) by a factor of 4
  // will find the constant factor 4 and produce X*(Y*Z).  Descaling X*(Y*8) by
  // a factor of 4 will produce X*(Y*2).  The principle of operation is to bore
  // down from Val:
  //
  //     Val = M1 * X          ||   Analysis starts here and works down
  //      M1 = M2 * Y          ||   Doesn't descend into terms with more
  //      M2 =  Z * 4          \/   than one use
  //
  // Then to modify a term at the bottom:
  //
  //     Val = M1 * X
  //      M1 =  Z * Y          ||   Replaced M2 with Z
  //
  // Then to work back up correcting nsw flags.

  // Op - the term we are currently analyzing.  Starts at Val then drills down.
  // Replaced with its descaled value before exiting from the drill down loop.
  Value *Op = Val;

  // Parent - initially null, but after drilling down notes where Op came from.
  // In the example above, Parent is (Val, 0) when Op is M1, because M1 is the
  // 0'th operand of Val.
  std::pair<Instruction*, unsigned> Parent;

  // RequireNoSignedWrap - Set if the transform requires a descaling at deeper
  // levels that doesn't overflow.
  bool RequireNoSignedWrap = false;

  // logScale - log base 2 of the scale.  Negative if not a power of 2.
  int32_t logScale = Scale.exactLogBase2();

  for (;; Op = Parent.first->getOperand(Parent.second)) { // Drill down

    if (ConstantInt *CI = dyn_cast<ConstantInt>(Op)) {
      // If Op is a constant divisible by Scale then descale to the quotient.
      APInt Quotient(Scale), Remainder(Scale); // Init ensures right bitwidth.
      APInt::sdivrem(CI->getValue(), Scale, Quotient, Remainder);
      if (!Remainder.isMinValue())
        // Not divisible by Scale.
        return 0;
      // Replace with the quotient in the parent.
      Op = ConstantInt::get(CI->getType(), Quotient);
      NoSignedWrap = true;
      break;
    }

    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Op)) {

      if (BO->getOpcode() == Instruction::Mul) {
        // Multiplication.
        NoSignedWrap = BO->hasNoSignedWrap();
        if (RequireNoSignedWrap && !NoSignedWrap)
          return 0;

        // There are three cases for multiplication: multiplication by exactly
        // the scale, multiplication by a constant different to the scale, and
        // multiplication by something else.
        Value *LHS = BO->getOperand(0);
        Value *RHS = BO->getOperand(1);

        if (ConstantInt *CI = dyn_cast<ConstantInt>(RHS)) {
          // Multiplication by a constant.
          if (CI->getValue() == Scale) {
            // Multiplication by exactly the scale, replace the multiplication
            // by its left-hand side in the parent.
            Op = LHS;
            break;
          }

          // Otherwise drill down into the constant.
          if (!Op->hasOneUse())
            return 0;

          Parent = std::make_pair(BO, 1);
          continue;
        }

        // Multiplication by something else. Drill down into the left-hand side
        // since that's where the reassociate pass puts the good stuff.
        if (!Op->hasOneUse())
          return 0;

        Parent = std::make_pair(BO, 0);
        continue;
      }

      if (logScale > 0 && BO->getOpcode() == Instruction::Shl &&
          isa<ConstantInt>(BO->getOperand(1))) {
        // Multiplication by a power of 2.
        NoSignedWrap = BO->hasNoSignedWrap();
        if (RequireNoSignedWrap && !NoSignedWrap)
          return 0;

        Value *LHS = BO->getOperand(0);
        int32_t Amt = cast<ConstantInt>(BO->getOperand(1))->
          getLimitedValue(Scale.getBitWidth());
        // Op = LHS << Amt.

        if (Amt == logScale) {
          // Multiplication by exactly the scale, replace the multiplication
          // by its left-hand side in the parent.
          Op = LHS;
          break;
        }
        if (Amt < logScale || !Op->hasOneUse())
          return 0;

        // Multiplication by more than the scale.  Reduce the multiplying amount
        // by the scale in the parent.
        Parent = std::make_pair(BO, 1);
        Op = ConstantInt::get(BO->getType(), Amt - logScale);
        break;
      }
    }

    if (!Op->hasOneUse())
      return 0;

    if (CastInst *Cast = dyn_cast<CastInst>(Op)) {
      if (Cast->getOpcode() == Instruction::SExt) {
        // Op is sign-extended from a smaller type, descale in the smaller type.
        unsigned SmallSize = Cast->getSrcTy()->getPrimitiveSizeInBits();
        APInt SmallScale = Scale.trunc(SmallSize);
        // Suppose Op = sext X, and we descale X as Y * SmallScale.  We want to
        // descale Op as (sext Y) * Scale.  In order to have
        //   sext (Y * SmallScale) = (sext Y) * Scale
        // some conditions need to hold however: SmallScale must sign-extend to
        // Scale and the multiplication Y * SmallScale should not overflow.
        if (SmallScale.sext(Scale.getBitWidth()) != Scale)
          // SmallScale does not sign-extend to Scale.
          return 0;
        assert(SmallScale.exactLogBase2() == logScale);
        // Require that Y * SmallScale must not overflow.
        RequireNoSignedWrap = true;

        // Drill down through the cast.
        Parent = std::make_pair(Cast, 0);
        Scale = SmallScale;
        continue;
      }

      if (Cast->getOpcode() == Instruction::Trunc) {
        // Op is truncated from a larger type, descale in the larger type.
        // Suppose Op = trunc X, and we descale X as Y * sext Scale.  Then
        //   trunc (Y * sext Scale) = (trunc Y) * Scale
        // always holds.  However (trunc Y) * Scale may overflow even if
        // trunc (Y * sext Scale) does not, so nsw flags need to be cleared
        // from this point up in the expression (see later).
        if (RequireNoSignedWrap)
          return 0;

        // Drill down through the cast.
        unsigned LargeSize = Cast->getSrcTy()->getPrimitiveSizeInBits();
        Parent = std::make_pair(Cast, 0);
        Scale = Scale.sext(LargeSize);
        if (logScale + 1 == (int32_t)Cast->getType()->getPrimitiveSizeInBits())
          logScale = -1;
        assert(Scale.exactLogBase2() == logScale);
        continue;
      }
    }

    // Unsupported expression, bail out.
    return 0;
  }

  // We know that we can successfully descale, so from here on we can safely
  // modify the IR.  Op holds the descaled version of the deepest term in the
  // expression.  NoSignedWrap is 'true' if multiplying Op by Scale is known
  // not to overflow.

  if (!Parent.first)
    // The expression only had one term.
    return Op;

  // Rewrite the parent using the descaled version of its operand.
  assert(Parent.first->hasOneUse() && "Drilled down when more than one use!");
  assert(Op != Parent.first->getOperand(Parent.second) &&
         "Descaling was a no-op?");
  Parent.first->setOperand(Parent.second, Op);
  Worklist.Add(Parent.first);

  // Now work back up the expression correcting nsw flags.  The logic is based
  // on the following observation: if X * Y is known not to overflow as a signed
  // multiplication, and Y is replaced by a value Z with smaller absolute value,
  // then X * Z will not overflow as a signed multiplication either.  As we work
  // our way up, having NoSignedWrap 'true' means that the descaled value at the
  // current level has strictly smaller absolute value than the original.
  Instruction *Ancestor = Parent.first;
  do {
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Ancestor)) {
      // If the multiplication wasn't nsw then we can't say anything about the
      // value of the descaled multiplication, and we have to clear nsw flags
      // from this point on up.
      bool OpNoSignedWrap = BO->hasNoSignedWrap();
      NoSignedWrap &= OpNoSignedWrap;
      if (NoSignedWrap != OpNoSignedWrap) {
        BO->setHasNoSignedWrap(NoSignedWrap);
        Worklist.Add(Ancestor);
      }
    } else if (Ancestor->getOpcode() == Instruction::Trunc) {
      // The fact that the descaled input to the trunc has smaller absolute
      // value than the original input doesn't tell us anything useful about
      // the absolute values of the truncations.
      NoSignedWrap = false;
    }
    assert((Ancestor->getOpcode() != Instruction::SExt || NoSignedWrap) &&
           "Failed to keep proper track of nsw flags while drilling down?");

    if (Ancestor == Val)
      // Got to the top, all done!
      return Val;

    // Move up one level in the expression.
    assert(Ancestor->hasOneUse() && "Drilled down when more than one use!");
    Ancestor = Ancestor->use_back();
  } while (1);
}

Instruction *InstCombiner::visitGetElementPtrInst(GetElementPtrInst &GEP) {
  SmallVector<Value*, 8> Ops(GEP.op_begin(), GEP.op_end());

  if (Value *V = SimplifyGEPInst(Ops, DL))
    return ReplaceInstUsesWith(GEP, V);

  Value *PtrOp = GEP.getOperand(0);

  // Eliminate unneeded casts for indices, and replace indices which displace
  // by multiples of a zero size type with zero.
  if (DL) {
    bool MadeChange = false;
    Type *IntPtrTy = DL->getIntPtrType(GEP.getPointerOperandType());

    gep_type_iterator GTI = gep_type_begin(GEP);
    for (User::op_iterator I = GEP.op_begin() + 1, E = GEP.op_end();
         I != E; ++I, ++GTI) {
      // Skip indices into struct types.
      SequentialType *SeqTy = dyn_cast<SequentialType>(*GTI);
      if (!SeqTy) continue;

      // If the element type has zero size then any index over it is equivalent
      // to an index of zero, so replace it with zero if it is not zero already.
      if (SeqTy->getElementType()->isSized() &&
          DL->getTypeAllocSize(SeqTy->getElementType()) == 0)
        if (!isa<Constant>(*I) || !cast<Constant>(*I)->isNullValue()) {
          *I = Constant::getNullValue(IntPtrTy);
          MadeChange = true;
        }

      Type *IndexTy = (*I)->getType();
      if (IndexTy != IntPtrTy) {
        // If we are using a wider index than needed for this platform, shrink
        // it to what we need.  If narrower, sign-extend it to what we need.
        // This explicit cast can make subsequent optimizations more obvious.
        *I = Builder->CreateIntCast(*I, IntPtrTy, true);
        MadeChange = true;
      }
    }
    if (MadeChange) return &GEP;
  }

  // Combine Indices - If the source pointer to this getelementptr instruction
  // is a getelementptr instruction, combine the indices of the two
  // getelementptr instructions into a single instruction.
  //
  if (GEPOperator *Src = dyn_cast<GEPOperator>(PtrOp)) {
    if (!shouldMergeGEPs(*cast<GEPOperator>(&GEP), *Src))
      return 0;

    // Note that if our source is a gep chain itself then we wait for that
    // chain to be resolved before we perform this transformation.  This
    // avoids us creating a TON of code in some cases.
    if (GEPOperator *SrcGEP =
          dyn_cast<GEPOperator>(Src->getOperand(0)))
      if (SrcGEP->getNumOperands() == 2 && shouldMergeGEPs(*Src, *SrcGEP))
        return 0;   // Wait until our source is folded to completion.

    SmallVector<Value*, 8> Indices;

    // Find out whether the last index in the source GEP is a sequential idx.
    bool EndsWithSequential = false;
    for (gep_type_iterator I = gep_type_begin(*Src), E = gep_type_end(*Src);
         I != E; ++I)
      EndsWithSequential = !(*I)->isStructTy();

    // Can we combine the two pointer arithmetics offsets?
    if (EndsWithSequential) {
      // Replace: gep (gep %P, long B), long A, ...
      // With:    T = long A+B; gep %P, T, ...
      //
      Value *Sum;
      Value *SO1 = Src->getOperand(Src->getNumOperands()-1);
      Value *GO1 = GEP.getOperand(1);
      if (SO1 == Constant::getNullValue(SO1->getType())) {
        Sum = GO1;
      } else if (GO1 == Constant::getNullValue(GO1->getType())) {
        Sum = SO1;
      } else {
        // If they aren't the same type, then the input hasn't been processed
        // by the loop above yet (which canonicalizes sequential index types to
        // intptr_t).  Just avoid transforming this until the input has been
        // normalized.
        if (SO1->getType() != GO1->getType())
          return 0;
        Sum = Builder->CreateAdd(SO1, GO1, PtrOp->getName()+".sum");
      }

      // Update the GEP in place if possible.
      if (Src->getNumOperands() == 2) {
        GEP.setOperand(0, Src->getOperand(0));
        GEP.setOperand(1, Sum);
        return &GEP;
      }
      Indices.append(Src->op_begin()+1, Src->op_end()-1);
      Indices.push_back(Sum);
      Indices.append(GEP.op_begin()+2, GEP.op_end());
    } else if (isa<Constant>(*GEP.idx_begin()) &&
               cast<Constant>(*GEP.idx_begin())->isNullValue() &&
               Src->getNumOperands() != 1) {
      // Otherwise we can do the fold if the first index of the GEP is a zero
      Indices.append(Src->op_begin()+1, Src->op_end());
      Indices.append(GEP.idx_begin()+1, GEP.idx_end());
    }

    if (!Indices.empty())
      return (GEP.isInBounds() && Src->isInBounds()) ?
        GetElementPtrInst::CreateInBounds(Src->getOperand(0), Indices,
                                          GEP.getName()) :
        GetElementPtrInst::Create(Src->getOperand(0), Indices, GEP.getName());
  }

  // Canonicalize (gep i8* X, -(ptrtoint Y)) to (sub (ptrtoint X), (ptrtoint Y))
  // The GEP pattern is emitted by the SCEV expander for certain kinds of
  // pointer arithmetic.
  if (DL && GEP.getNumIndices() == 1 &&
      match(GEP.getOperand(1), m_Neg(m_PtrToInt(m_Value())))) {
    unsigned AS = GEP.getPointerAddressSpace();
    if (GEP.getType() == Builder->getInt8PtrTy(AS) &&
        GEP.getOperand(1)->getType()->getScalarSizeInBits() ==
        DL->getPointerSizeInBits(AS)) {
      Operator *Index = cast<Operator>(GEP.getOperand(1));
      Value *PtrToInt = Builder->CreatePtrToInt(PtrOp, Index->getType());
      Value *NewSub = Builder->CreateSub(PtrToInt, Index->getOperand(1));
      return CastInst::Create(Instruction::IntToPtr, NewSub, GEP.getType());
    }
  }

  // Handle gep(bitcast x) and gep(gep x, 0, 0, 0).
  Value *StrippedPtr = PtrOp->stripPointerCasts();
  PointerType *StrippedPtrTy = dyn_cast<PointerType>(StrippedPtr->getType());

  // We do not handle pointer-vector geps here.
  if (!StrippedPtrTy)
    return 0;

  if (StrippedPtr != PtrOp) {
    bool HasZeroPointerIndex = false;
    if (ConstantInt *C = dyn_cast<ConstantInt>(GEP.getOperand(1)))
      HasZeroPointerIndex = C->isZero();

    // Transform: GEP (bitcast [10 x i8]* X to [0 x i8]*), i32 0, ...
    // into     : GEP [10 x i8]* X, i32 0, ...
    //
    // Likewise, transform: GEP (bitcast i8* X to [0 x i8]*), i32 0, ...
    //           into     : GEP i8* X, ...
    //
    // This occurs when the program declares an array extern like "int X[];"
    if (HasZeroPointerIndex) {
      PointerType *CPTy = cast<PointerType>(PtrOp->getType());
      if (ArrayType *CATy =
          dyn_cast<ArrayType>(CPTy->getElementType())) {
        // GEP (bitcast i8* X to [0 x i8]*), i32 0, ... ?
        if (CATy->getElementType() == StrippedPtrTy->getElementType()) {
          // -> GEP i8* X, ...
          SmallVector<Value*, 8> Idx(GEP.idx_begin()+1, GEP.idx_end());
          GetElementPtrInst *Res =
            GetElementPtrInst::Create(StrippedPtr, Idx, GEP.getName());
          Res->setIsInBounds(GEP.isInBounds());
          return Res;
        }

        if (ArrayType *XATy =
              dyn_cast<ArrayType>(StrippedPtrTy->getElementType())){
          // GEP (bitcast [10 x i8]* X to [0 x i8]*), i32 0, ... ?
          if (CATy->getElementType() == XATy->getElementType()) {
            // -> GEP [10 x i8]* X, i32 0, ...
            // At this point, we know that the cast source type is a pointer
            // to an array of the same type as the destination pointer
            // array.  Because the array type is never stepped over (there
            // is a leading zero) we can fold the cast into this GEP.
            GEP.setOperand(0, StrippedPtr);
            return &GEP;
          }
        }
      }
    } else if (GEP.getNumOperands() == 2) {
      // Transform things like:
      // %t = getelementptr i32* bitcast ([2 x i32]* %str to i32*), i32 %V
      // into:  %t1 = getelementptr [2 x i32]* %str, i32 0, i32 %V; bitcast
      Type *SrcElTy = StrippedPtrTy->getElementType();
      Type *ResElTy = PtrOp->getType()->getPointerElementType();
      if (DL && SrcElTy->isArrayTy() &&
          DL->getTypeAllocSize(SrcElTy->getArrayElementType()) ==
          DL->getTypeAllocSize(ResElTy)) {
        Type *IdxType = DL->getIntPtrType(GEP.getType());
        Value *Idx[2] = { Constant::getNullValue(IdxType), GEP.getOperand(1) };
        Value *NewGEP = GEP.isInBounds() ?
          Builder->CreateInBoundsGEP(StrippedPtr, Idx, GEP.getName()) :
          Builder->CreateGEP(StrippedPtr, Idx, GEP.getName());

        // V and GEP are both pointer types --> BitCast
        if (StrippedPtrTy->getAddressSpace() == GEP.getPointerAddressSpace())
          return new BitCastInst(NewGEP, GEP.getType());
        return new AddrSpaceCastInst(NewGEP, GEP.getType());
      }

      // Transform things like:
      // %V = mul i64 %N, 4
      // %t = getelementptr i8* bitcast (i32* %arr to i8*), i32 %V
      // into:  %t1 = getelementptr i32* %arr, i32 %N; bitcast
      if (DL && ResElTy->isSized() && SrcElTy->isSized()) {
        // Check that changing the type amounts to dividing the index by a scale
        // factor.
        uint64_t ResSize = DL->getTypeAllocSize(ResElTy);
        uint64_t SrcSize = DL->getTypeAllocSize(SrcElTy);
        if (ResSize && SrcSize % ResSize == 0) {
          Value *Idx = GEP.getOperand(1);
          unsigned BitWidth = Idx->getType()->getPrimitiveSizeInBits();
          uint64_t Scale = SrcSize / ResSize;

          // Earlier transforms ensure that the index has type IntPtrType, which
          // considerably simplifies the logic by eliminating implicit casts.
          assert(Idx->getType() == DL->getIntPtrType(GEP.getType()) &&
                 "Index not cast to pointer width?");

          bool NSW;
          if (Value *NewIdx = Descale(Idx, APInt(BitWidth, Scale), NSW)) {
            // Successfully decomposed Idx as NewIdx * Scale, form a new GEP.
            // If the multiplication NewIdx * Scale may overflow then the new
            // GEP may not be "inbounds".
            Value *NewGEP = GEP.isInBounds() && NSW ?
              Builder->CreateInBoundsGEP(StrippedPtr, NewIdx, GEP.getName()) :
              Builder->CreateGEP(StrippedPtr, NewIdx, GEP.getName());

            // The NewGEP must be pointer typed, so must the old one -> BitCast
            if (StrippedPtrTy->getAddressSpace() == GEP.getPointerAddressSpace())
              return new BitCastInst(NewGEP, GEP.getType());
            return new AddrSpaceCastInst(NewGEP, GEP.getType());
          }
        }
      }

      // Similarly, transform things like:
      // getelementptr i8* bitcast ([100 x double]* X to i8*), i32 %tmp
      //   (where tmp = 8*tmp2) into:
      // getelementptr [100 x double]* %arr, i32 0, i32 %tmp2; bitcast
      if (DL && ResElTy->isSized() && SrcElTy->isSized() &&
          SrcElTy->isArrayTy()) {
        // Check that changing to the array element type amounts to dividing the
        // index by a scale factor.
        uint64_t ResSize = DL->getTypeAllocSize(ResElTy);
        uint64_t ArrayEltSize
          = DL->getTypeAllocSize(SrcElTy->getArrayElementType());
        if (ResSize && ArrayEltSize % ResSize == 0) {
          Value *Idx = GEP.getOperand(1);
          unsigned BitWidth = Idx->getType()->getPrimitiveSizeInBits();
          uint64_t Scale = ArrayEltSize / ResSize;

          // Earlier transforms ensure that the index has type IntPtrType, which
          // considerably simplifies the logic by eliminating implicit casts.
          assert(Idx->getType() == DL->getIntPtrType(GEP.getType()) &&
                 "Index not cast to pointer width?");

          bool NSW;
          if (Value *NewIdx = Descale(Idx, APInt(BitWidth, Scale), NSW)) {
            // Successfully decomposed Idx as NewIdx * Scale, form a new GEP.
            // If the multiplication NewIdx * Scale may overflow then the new
            // GEP may not be "inbounds".
            Value *Off[2] = {
              Constant::getNullValue(DL->getIntPtrType(GEP.getType())),
              NewIdx
            };

            Value *NewGEP = GEP.isInBounds() && NSW ?
              Builder->CreateInBoundsGEP(StrippedPtr, Off, GEP.getName()) :
              Builder->CreateGEP(StrippedPtr, Off, GEP.getName());
            // The NewGEP must be pointer typed, so must the old one -> BitCast
            if (StrippedPtrTy->getAddressSpace() == GEP.getPointerAddressSpace())
              return new BitCastInst(NewGEP, GEP.getType());
            return new AddrSpaceCastInst(NewGEP, GEP.getType());
          }
        }
      }
    }
  }

  if (!DL)
    return 0;

  /// See if we can simplify:
  ///   X = bitcast A* to B*
  ///   Y = gep X, <...constant indices...>
  /// into a gep of the original struct.  This is important for SROA and alias
  /// analysis of unions.  If "A" is also a bitcast, wait for A/X to be merged.
  if (BitCastInst *BCI = dyn_cast<BitCastInst>(PtrOp)) {
    Value *Operand = BCI->getOperand(0);
    PointerType *OpType = cast<PointerType>(Operand->getType());
    unsigned OffsetBits = DL->getPointerTypeSizeInBits(OpType);
    APInt Offset(OffsetBits, 0);
    if (!isa<BitCastInst>(Operand) &&
        GEP.accumulateConstantOffset(*DL, Offset) &&
        StrippedPtrTy->getAddressSpace() == GEP.getPointerAddressSpace()) {

      // If this GEP instruction doesn't move the pointer, just replace the GEP
      // with a bitcast of the real input to the dest type.
      if (!Offset) {
        // If the bitcast is of an allocation, and the allocation will be
        // converted to match the type of the cast, don't touch this.
        if (isa<AllocaInst>(Operand) || isAllocationFn(Operand, TLI)) {
          // See if the bitcast simplifies, if so, don't nuke this GEP yet.
          if (Instruction *I = visitBitCast(*BCI)) {
            if (I != BCI) {
              I->takeName(BCI);
              BCI->getParent()->getInstList().insert(BCI, I);
              ReplaceInstUsesWith(*BCI, I);
            }
            return &GEP;
          }
        }
        return new BitCastInst(Operand, GEP.getType());
      }

      // Otherwise, if the offset is non-zero, we need to find out if there is a
      // field at Offset in 'A's type.  If so, we can pull the cast through the
      // GEP.
      SmallVector<Value*, 8> NewIndices;
      if (FindElementAtOffset(OpType, Offset.getSExtValue(), NewIndices)) {
        Value *NGEP = GEP.isInBounds() ?
          Builder->CreateInBoundsGEP(Operand, NewIndices) :
          Builder->CreateGEP(Operand, NewIndices);

        if (NGEP->getType() == GEP.getType())
          return ReplaceInstUsesWith(GEP, NGEP);
        NGEP->takeName(&GEP);
        return new BitCastInst(NGEP, GEP.getType());
      }
    }
  }

  return 0;
}

static bool
isAllocSiteRemovable(Instruction *AI, SmallVectorImpl<WeakVH> &Users,
                     const TargetLibraryInfo *TLI) {
  SmallVector<Instruction*, 4> Worklist;
  Worklist.push_back(AI);

  do {
    Instruction *PI = Worklist.pop_back_val();
    for (Value::use_iterator UI = PI->use_begin(), UE = PI->use_end(); UI != UE;
         ++UI) {
      Instruction *I = cast<Instruction>(*UI);
      switch (I->getOpcode()) {
      default:
        // Give up the moment we see something we can't handle.
        return false;

      case Instruction::BitCast:
      case Instruction::GetElementPtr:
        Users.push_back(I);
        Worklist.push_back(I);
        continue;

      case Instruction::ICmp: {
        ICmpInst *ICI = cast<ICmpInst>(I);
        // We can fold eq/ne comparisons with null to false/true, respectively.
        if (!ICI->isEquality() || !isa<ConstantPointerNull>(ICI->getOperand(1)))
          return false;
        Users.push_back(I);
        continue;
      }

      case Instruction::Call:
        // Ignore no-op and store intrinsics.
        if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(I)) {
          switch (II->getIntrinsicID()) {
          default:
            return false;

          case Intrinsic::memmove:
          case Intrinsic::memcpy:
          case Intrinsic::memset: {
            MemIntrinsic *MI = cast<MemIntrinsic>(II);
            if (MI->isVolatile() || MI->getRawDest() != PI)
              return false;
          }
          // fall through
          case Intrinsic::dbg_declare:
          case Intrinsic::dbg_value:
          case Intrinsic::invariant_start:
          case Intrinsic::invariant_end:
          case Intrinsic::lifetime_start:
          case Intrinsic::lifetime_end:
          case Intrinsic::objectsize:
            Users.push_back(I);
            continue;
          }
        }

        if (isFreeCall(I, TLI)) {
          Users.push_back(I);
          continue;
        }
        return false;

      case Instruction::Store: {
        StoreInst *SI = cast<StoreInst>(I);
        if (SI->isVolatile() || SI->getPointerOperand() != PI)
          return false;
        Users.push_back(I);
        continue;
      }
      }
      llvm_unreachable("missing a return?");
    }
  } while (!Worklist.empty());
  return true;
}

Instruction *InstCombiner::visitAllocSite(Instruction &MI) {
  // If we have a malloc call which is only used in any amount of comparisons
  // to null and free calls, delete the calls and replace the comparisons with
  // true or false as appropriate.
  SmallVector<WeakVH, 64> Users;
  if (isAllocSiteRemovable(&MI, Users, TLI)) {
    for (unsigned i = 0, e = Users.size(); i != e; ++i) {
      Instruction *I = cast_or_null<Instruction>(&*Users[i]);
      if (!I) continue;

      if (ICmpInst *C = dyn_cast<ICmpInst>(I)) {
        ReplaceInstUsesWith(*C,
                            ConstantInt::get(Type::getInt1Ty(C->getContext()),
                                             C->isFalseWhenEqual()));
      } else if (isa<BitCastInst>(I) || isa<GetElementPtrInst>(I)) {
        ReplaceInstUsesWith(*I, UndefValue::get(I->getType()));
      } else if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(I)) {
        if (II->getIntrinsicID() == Intrinsic::objectsize) {
          ConstantInt *CI = cast<ConstantInt>(II->getArgOperand(1));
          uint64_t DontKnow = CI->isZero() ? -1ULL : 0;
          ReplaceInstUsesWith(*I, ConstantInt::get(I->getType(), DontKnow));
        }
      }
      EraseInstFromFunction(*I);
    }

    if (InvokeInst *II = dyn_cast<InvokeInst>(&MI)) {
      // Replace invoke with a NOP intrinsic to maintain the original CFG
      Module *M = II->getParent()->getParent()->getParent();
      Function *F = Intrinsic::getDeclaration(M, Intrinsic::donothing);
      InvokeInst::Create(F, II->getNormalDest(), II->getUnwindDest(),
                         None, "", II->getParent());
    }
    return EraseInstFromFunction(MI);
  }
  return 0;
}

/// \brief Move the call to free before a NULL test.
///
/// Check if this free is accessed after its argument has been test
/// against NULL (property 0).
/// If yes, it is legal to move this call in its predecessor block.
///
/// The move is performed only if the block containing the call to free
/// will be removed, i.e.:
/// 1. it has only one predecessor P, and P has two successors
/// 2. it contains the call and an unconditional branch
/// 3. its successor is the same as its predecessor's successor
///
/// The profitability is out-of concern here and this function should
/// be called only if the caller knows this transformation would be
/// profitable (e.g., for code size).
static Instruction *
tryToMoveFreeBeforeNullTest(CallInst &FI) {
  Value *Op = FI.getArgOperand(0);
  BasicBlock *FreeInstrBB = FI.getParent();
  BasicBlock *PredBB = FreeInstrBB->getSinglePredecessor();

  // Validate part of constraint #1: Only one predecessor
  // FIXME: We can extend the number of predecessor, but in that case, we
  //        would duplicate the call to free in each predecessor and it may
  //        not be profitable even for code size.
  if (!PredBB)
    return 0;

  // Validate constraint #2: Does this block contains only the call to
  //                         free and an unconditional branch?
  // FIXME: We could check if we can speculate everything in the
  //        predecessor block
  if (FreeInstrBB->size() != 2)
    return 0;
  BasicBlock *SuccBB;
  if (!match(FreeInstrBB->getTerminator(), m_UnconditionalBr(SuccBB)))
    return 0;

  // Validate the rest of constraint #1 by matching on the pred branch.
  TerminatorInst *TI = PredBB->getTerminator();
  BasicBlock *TrueBB, *FalseBB;
  ICmpInst::Predicate Pred;
  if (!match(TI, m_Br(m_ICmp(Pred, m_Specific(Op), m_Zero()), TrueBB, FalseBB)))
    return 0;
  if (Pred != ICmpInst::ICMP_EQ && Pred != ICmpInst::ICMP_NE)
    return 0;

  // Validate constraint #3: Ensure the null case just falls through.
  if (SuccBB != (Pred == ICmpInst::ICMP_EQ ? TrueBB : FalseBB))
    return 0;
  assert(FreeInstrBB == (Pred == ICmpInst::ICMP_EQ ? FalseBB : TrueBB) &&
         "Broken CFG: missing edge from predecessor to successor");

  FI.moveBefore(TI);
  return &FI;
}


Instruction *InstCombiner::visitFree(CallInst &FI) {
  Value *Op = FI.getArgOperand(0);

  // free undef -> unreachable.
  if (isa<UndefValue>(Op)) {
    // Insert a new store to null because we cannot modify the CFG here.
    Builder->CreateStore(ConstantInt::getTrue(FI.getContext()),
                         UndefValue::get(Type::getInt1PtrTy(FI.getContext())));
    return EraseInstFromFunction(FI);
  }

  // If we have 'free null' delete the instruction.  This can happen in stl code
  // when lots of inlining happens.
  if (isa<ConstantPointerNull>(Op))
    return EraseInstFromFunction(FI);

  // If we optimize for code size, try to move the call to free before the null
  // test so that simplify cfg can remove the empty block and dead code
  // elimination the branch. I.e., helps to turn something like:
  // if (foo) free(foo);
  // into
  // free(foo);
  if (MinimizeSize)
    if (Instruction *I = tryToMoveFreeBeforeNullTest(FI))
      return I;

  return 0;
}



Instruction *InstCombiner::visitBranchInst(BranchInst &BI) {
  // Change br (not X), label True, label False to: br X, label False, True
  Value *X = 0;
  BasicBlock *TrueDest;
  BasicBlock *FalseDest;
  if (match(&BI, m_Br(m_Not(m_Value(X)), TrueDest, FalseDest)) &&
      !isa<Constant>(X)) {
    // Swap Destinations and condition...
    BI.setCondition(X);
    BI.swapSuccessors();
    return &BI;
  }

  // Canonicalize fcmp_one -> fcmp_oeq
  FCmpInst::Predicate FPred; Value *Y;
  if (match(&BI, m_Br(m_FCmp(FPred, m_Value(X), m_Value(Y)),
                             TrueDest, FalseDest)) &&
      BI.getCondition()->hasOneUse())
    if (FPred == FCmpInst::FCMP_ONE || FPred == FCmpInst::FCMP_OLE ||
        FPred == FCmpInst::FCMP_OGE) {
      FCmpInst *Cond = cast<FCmpInst>(BI.getCondition());
      Cond->setPredicate(FCmpInst::getInversePredicate(FPred));

      // Swap Destinations and condition.
      BI.swapSuccessors();
      Worklist.Add(Cond);
      return &BI;
    }

  // Canonicalize icmp_ne -> icmp_eq
  ICmpInst::Predicate IPred;
  if (match(&BI, m_Br(m_ICmp(IPred, m_Value(X), m_Value(Y)),
                      TrueDest, FalseDest)) &&
      BI.getCondition()->hasOneUse())
    if (IPred == ICmpInst::ICMP_NE  || IPred == ICmpInst::ICMP_ULE ||
        IPred == ICmpInst::ICMP_SLE || IPred == ICmpInst::ICMP_UGE ||
        IPred == ICmpInst::ICMP_SGE) {
      ICmpInst *Cond = cast<ICmpInst>(BI.getCondition());
      Cond->setPredicate(ICmpInst::getInversePredicate(IPred));
      // Swap Destinations and condition.
      BI.swapSuccessors();
      Worklist.Add(Cond);
      return &BI;
    }

  return 0;
}

Instruction *InstCombiner::visitSwitchInst(SwitchInst &SI) {
  Value *Cond = SI.getCondition();
  if (Instruction *I = dyn_cast<Instruction>(Cond)) {
    if (I->getOpcode() == Instruction::Add)
      if (ConstantInt *AddRHS = dyn_cast<ConstantInt>(I->getOperand(1))) {
        // change 'switch (X+4) case 1:' into 'switch (X) case -3'
        // Skip the first item since that's the default case.
        for (SwitchInst::CaseIt i = SI.case_begin(), e = SI.case_end();
             i != e; ++i) {
          ConstantInt* CaseVal = i.getCaseValue();
          Constant* NewCaseVal = ConstantExpr::getSub(cast<Constant>(CaseVal),
                                                      AddRHS);
          assert(isa<ConstantInt>(NewCaseVal) &&
                 "Result of expression should be constant");
          i.setValue(cast<ConstantInt>(NewCaseVal));
        }
        SI.setCondition(I->getOperand(0));
        Worklist.Add(I);
        return &SI;
      }
  }
  return 0;
}

Instruction *InstCombiner::visitExtractValueInst(ExtractValueInst &EV) {
  Value *Agg = EV.getAggregateOperand();

  if (!EV.hasIndices())
    return ReplaceInstUsesWith(EV, Agg);

  if (Constant *C = dyn_cast<Constant>(Agg)) {
    if (Constant *C2 = C->getAggregateElement(*EV.idx_begin())) {
      if (EV.getNumIndices() == 0)
        return ReplaceInstUsesWith(EV, C2);
      // Extract the remaining indices out of the constant indexed by the
      // first index
      return ExtractValueInst::Create(C2, EV.getIndices().slice(1));
    }
    return 0; // Can't handle other constants
  }

  if (InsertValueInst *IV = dyn_cast<InsertValueInst>(Agg)) {
    // We're extracting from an insertvalue instruction, compare the indices
    const unsigned *exti, *exte, *insi, *inse;
    for (exti = EV.idx_begin(), insi = IV->idx_begin(),
         exte = EV.idx_end(), inse = IV->idx_end();
         exti != exte && insi != inse;
         ++exti, ++insi) {
      if (*insi != *exti)
        // The insert and extract both reference distinctly different elements.
        // This means the extract is not influenced by the insert, and we can
        // replace the aggregate operand of the extract with the aggregate
        // operand of the insert. i.e., replace
        // %I = insertvalue { i32, { i32 } } %A, { i32 } { i32 42 }, 1
        // %E = extractvalue { i32, { i32 } } %I, 0
        // with
        // %E = extractvalue { i32, { i32 } } %A, 0
        return ExtractValueInst::Create(IV->getAggregateOperand(),
                                        EV.getIndices());
    }
    if (exti == exte && insi == inse)
      // Both iterators are at the end: Index lists are identical. Replace
      // %B = insertvalue { i32, { i32 } } %A, i32 42, 1, 0
      // %C = extractvalue { i32, { i32 } } %B, 1, 0
      // with "i32 42"
      return ReplaceInstUsesWith(EV, IV->getInsertedValueOperand());
    if (exti == exte) {
      // The extract list is a prefix of the insert list. i.e. replace
      // %I = insertvalue { i32, { i32 } } %A, i32 42, 1, 0
      // %E = extractvalue { i32, { i32 } } %I, 1
      // with
      // %X = extractvalue { i32, { i32 } } %A, 1
      // %E = insertvalue { i32 } %X, i32 42, 0
      // by switching the order of the insert and extract (though the
      // insertvalue should be left in, since it may have other uses).
      Value *NewEV = Builder->CreateExtractValue(IV->getAggregateOperand(),
                                                 EV.getIndices());
      return InsertValueInst::Create(NewEV, IV->getInsertedValueOperand(),
                                     makeArrayRef(insi, inse));
    }
    if (insi == inse)
      // The insert list is a prefix of the extract list
      // We can simply remove the common indices from the extract and make it
      // operate on the inserted value instead of the insertvalue result.
      // i.e., replace
      // %I = insertvalue { i32, { i32 } } %A, { i32 } { i32 42 }, 1
      // %E = extractvalue { i32, { i32 } } %I, 1, 0
      // with
      // %E extractvalue { i32 } { i32 42 }, 0
      return ExtractValueInst::Create(IV->getInsertedValueOperand(),
                                      makeArrayRef(exti, exte));
  }
  if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(Agg)) {
    // We're extracting from an intrinsic, see if we're the only user, which
    // allows us to simplify multiple result intrinsics to simpler things that
    // just get one value.
    if (II->hasOneUse()) {
      // Check if we're grabbing the overflow bit or the result of a 'with
      // overflow' intrinsic.  If it's the latter we can remove the intrinsic
      // and replace it with a traditional binary instruction.
      switch (II->getIntrinsicID()) {
      case Intrinsic::uadd_with_overflow:
      case Intrinsic::sadd_with_overflow:
        if (*EV.idx_begin() == 0) {  // Normal result.
          Value *LHS = II->getArgOperand(0), *RHS = II->getArgOperand(1);
          ReplaceInstUsesWith(*II, UndefValue::get(II->getType()));
          EraseInstFromFunction(*II);
          return BinaryOperator::CreateAdd(LHS, RHS);
        }

        // If the normal result of the add is dead, and the RHS is a constant,
        // we can transform this into a range comparison.
        // overflow = uadd a, -4  -->  overflow = icmp ugt a, 3
        if (II->getIntrinsicID() == Intrinsic::uadd_with_overflow)
          if (ConstantInt *CI = dyn_cast<ConstantInt>(II->getArgOperand(1)))
            return new ICmpInst(ICmpInst::ICMP_UGT, II->getArgOperand(0),
                                ConstantExpr::getNot(CI));
        break;
      case Intrinsic::usub_with_overflow:
      case Intrinsic::ssub_with_overflow:
        if (*EV.idx_begin() == 0) {  // Normal result.
          Value *LHS = II->getArgOperand(0), *RHS = II->getArgOperand(1);
          ReplaceInstUsesWith(*II, UndefValue::get(II->getType()));
          EraseInstFromFunction(*II);
          return BinaryOperator::CreateSub(LHS, RHS);
        }
        break;
      case Intrinsic::umul_with_overflow:
      case Intrinsic::smul_with_overflow:
        if (*EV.idx_begin() == 0) {  // Normal result.
          Value *LHS = II->getArgOperand(0), *RHS = II->getArgOperand(1);
          ReplaceInstUsesWith(*II, UndefValue::get(II->getType()));
          EraseInstFromFunction(*II);
          return BinaryOperator::CreateMul(LHS, RHS);
        }
        break;
      default:
        break;
      }
    }
  }
  if (LoadInst *L = dyn_cast<LoadInst>(Agg))
    // If the (non-volatile) load only has one use, we can rewrite this to a
    // load from a GEP. This reduces the size of the load.
    // FIXME: If a load is used only by extractvalue instructions then this
    //        could be done regardless of having multiple uses.
    if (L->isSimple() && L->hasOneUse()) {
      // extractvalue has integer indices, getelementptr has Value*s. Convert.
      SmallVector<Value*, 4> Indices;
      // Prefix an i32 0 since we need the first element.
      Indices.push_back(Builder->getInt32(0));
      for (ExtractValueInst::idx_iterator I = EV.idx_begin(), E = EV.idx_end();
            I != E; ++I)
        Indices.push_back(Builder->getInt32(*I));

      // We need to insert these at the location of the old load, not at that of
      // the extractvalue.
      Builder->SetInsertPoint(L->getParent(), L);
      Value *GEP = Builder->CreateInBoundsGEP(L->getPointerOperand(), Indices);
      // Returning the load directly will cause the main loop to insert it in
      // the wrong spot, so use ReplaceInstUsesWith().
      return ReplaceInstUsesWith(EV, Builder->CreateLoad(GEP));
    }
  // We could simplify extracts from other values. Note that nested extracts may
  // already be simplified implicitly by the above: extract (extract (insert) )
  // will be translated into extract ( insert ( extract ) ) first and then just
  // the value inserted, if appropriate. Similarly for extracts from single-use
  // loads: extract (extract (load)) will be translated to extract (load (gep))
  // and if again single-use then via load (gep (gep)) to load (gep).
  // However, double extracts from e.g. function arguments or return values
  // aren't handled yet.
  return 0;
}

enum Personality_Type {
  Unknown_Personality,
  GNU_Ada_Personality,
  GNU_CXX_Personality,
  GNU_ObjC_Personality
};

/// RecognizePersonality - See if the given exception handling personality
/// function is one that we understand.  If so, return a description of it;
/// otherwise return Unknown_Personality.
static Personality_Type RecognizePersonality(Value *Pers) {
  Function *F = dyn_cast<Function>(Pers->stripPointerCasts());
  if (!F)
    return Unknown_Personality;
  return StringSwitch<Personality_Type>(F->getName())
    .Case("__gnat_eh_personality", GNU_Ada_Personality)
    .Case("__gxx_personality_v0",  GNU_CXX_Personality)
    .Case("__objc_personality_v0", GNU_ObjC_Personality)
    .Default(Unknown_Personality);
}

/// isCatchAll - Return 'true' if the given typeinfo will match anything.
static bool isCatchAll(Personality_Type Personality, Constant *TypeInfo) {
  switch (Personality) {
  case Unknown_Personality:
    return false;
  case GNU_Ada_Personality:
    // While __gnat_all_others_value will match any Ada exception, it doesn't
    // match foreign exceptions (or didn't, before gcc-4.7).
    return false;
  case GNU_CXX_Personality:
  case GNU_ObjC_Personality:
    return TypeInfo->isNullValue();
  }
  llvm_unreachable("Unknown personality!");
}

static bool shorter_filter(const Value *LHS, const Value *RHS) {
  return
    cast<ArrayType>(LHS->getType())->getNumElements()
  <
    cast<ArrayType>(RHS->getType())->getNumElements();
}

Instruction *InstCombiner::visitLandingPadInst(LandingPadInst &LI) {
  // The logic here should be correct for any real-world personality function.
  // However if that turns out not to be true, the offending logic can always
  // be conditioned on the personality function, like the catch-all logic is.
  Personality_Type Personality = RecognizePersonality(LI.getPersonalityFn());

  // Simplify the list of clauses, eg by removing repeated catch clauses
  // (these are often created by inlining).
  bool MakeNewInstruction = false; // If true, recreate using the following:
  SmallVector<Value *, 16> NewClauses; // - Clauses for the new instruction;
  bool CleanupFlag = LI.isCleanup();   // - The new instruction is a cleanup.

  SmallPtrSet<Value *, 16> AlreadyCaught; // Typeinfos known caught already.
  for (unsigned i = 0, e = LI.getNumClauses(); i != e; ++i) {
    bool isLastClause = i + 1 == e;
    if (LI.isCatch(i)) {
      // A catch clause.
      Value *CatchClause = LI.getClause(i);
      Constant *TypeInfo = cast<Constant>(CatchClause->stripPointerCasts());

      // If we already saw this clause, there is no point in having a second
      // copy of it.
      if (AlreadyCaught.insert(TypeInfo)) {
        // This catch clause was not already seen.
        NewClauses.push_back(CatchClause);
      } else {
        // Repeated catch clause - drop the redundant copy.
        MakeNewInstruction = true;
      }

      // If this is a catch-all then there is no point in keeping any following
      // clauses or marking the landingpad as having a cleanup.
      if (isCatchAll(Personality, TypeInfo)) {
        if (!isLastClause)
          MakeNewInstruction = true;
        CleanupFlag = false;
        break;
      }
    } else {
      // A filter clause.  If any of the filter elements were already caught
      // then they can be dropped from the filter.  It is tempting to try to
      // exploit the filter further by saying that any typeinfo that does not
      // occur in the filter can't be caught later (and thus can be dropped).
      // However this would be wrong, since typeinfos can match without being
      // equal (for example if one represents a C++ class, and the other some
      // class derived from it).
      assert(LI.isFilter(i) && "Unsupported landingpad clause!");
      Value *FilterClause = LI.getClause(i);
      ArrayType *FilterType = cast<ArrayType>(FilterClause->getType());
      unsigned NumTypeInfos = FilterType->getNumElements();

      // An empty filter catches everything, so there is no point in keeping any
      // following clauses or marking the landingpad as having a cleanup.  By
      // dealing with this case here the following code is made a bit simpler.
      if (!NumTypeInfos) {
        NewClauses.push_back(FilterClause);
        if (!isLastClause)
          MakeNewInstruction = true;
        CleanupFlag = false;
        break;
      }

      bool MakeNewFilter = false; // If true, make a new filter.
      SmallVector<Constant *, 16> NewFilterElts; // New elements.
      if (isa<ConstantAggregateZero>(FilterClause)) {
        // Not an empty filter - it contains at least one null typeinfo.
        assert(NumTypeInfos > 0 && "Should have handled empty filter already!");
        Constant *TypeInfo =
          Constant::getNullValue(FilterType->getElementType());
        // If this typeinfo is a catch-all then the filter can never match.
        if (isCatchAll(Personality, TypeInfo)) {
          // Throw the filter away.
          MakeNewInstruction = true;
          continue;
        }

        // There is no point in having multiple copies of this typeinfo, so
        // discard all but the first copy if there is more than one.
        NewFilterElts.push_back(TypeInfo);
        if (NumTypeInfos > 1)
          MakeNewFilter = true;
      } else {
        ConstantArray *Filter = cast<ConstantArray>(FilterClause);
        SmallPtrSet<Value *, 16> SeenInFilter; // For uniquing the elements.
        NewFilterElts.reserve(NumTypeInfos);

        // Remove any filter elements that were already caught or that already
        // occurred in the filter.  While there, see if any of the elements are
        // catch-alls.  If so, the filter can be discarded.
        bool SawCatchAll = false;
        for (unsigned j = 0; j != NumTypeInfos; ++j) {
          Value *Elt = Filter->getOperand(j);
          Constant *TypeInfo = cast<Constant>(Elt->stripPointerCasts());
          if (isCatchAll(Personality, TypeInfo)) {
            // This element is a catch-all.  Bail out, noting this fact.
            SawCatchAll = true;
            break;
          }
          if (AlreadyCaught.count(TypeInfo))
            // Already caught by an earlier clause, so having it in the filter
            // is pointless.
            continue;
          // There is no point in having multiple copies of the same typeinfo in
          // a filter, so only add it if we didn't already.
          if (SeenInFilter.insert(TypeInfo))
            NewFilterElts.push_back(cast<Constant>(Elt));
        }
        // A filter containing a catch-all cannot match anything by definition.
        if (SawCatchAll) {
          // Throw the filter away.
          MakeNewInstruction = true;
          continue;
        }

        // If we dropped something from the filter, make a new one.
        if (NewFilterElts.size() < NumTypeInfos)
          MakeNewFilter = true;
      }
      if (MakeNewFilter) {
        FilterType = ArrayType::get(FilterType->getElementType(),
                                    NewFilterElts.size());
        FilterClause = ConstantArray::get(FilterType, NewFilterElts);
        MakeNewInstruction = true;
      }

      NewClauses.push_back(FilterClause);

      // If the new filter is empty then it will catch everything so there is
      // no point in keeping any following clauses or marking the landingpad
      // as having a cleanup.  The case of the original filter being empty was
      // already handled above.
      if (MakeNewFilter && !NewFilterElts.size()) {
        assert(MakeNewInstruction && "New filter but not a new instruction!");
        CleanupFlag = false;
        break;
      }
    }
  }

  // If several filters occur in a row then reorder them so that the shortest
  // filters come first (those with the smallest number of elements).  This is
  // advantageous because shorter filters are more likely to match, speeding up
  // unwinding, but mostly because it increases the effectiveness of the other
  // filter optimizations below.
  for (unsigned i = 0, e = NewClauses.size(); i + 1 < e; ) {
    unsigned j;
    // Find the maximal 'j' s.t. the range [i, j) consists entirely of filters.
    for (j = i; j != e; ++j)
      if (!isa<ArrayType>(NewClauses[j]->getType()))
        break;

    // Check whether the filters are already sorted by length.  We need to know
    // if sorting them is actually going to do anything so that we only make a
    // new landingpad instruction if it does.
    for (unsigned k = i; k + 1 < j; ++k)
      if (shorter_filter(NewClauses[k+1], NewClauses[k])) {
        // Not sorted, so sort the filters now.  Doing an unstable sort would be
        // correct too but reordering filters pointlessly might confuse users.
        std::stable_sort(NewClauses.begin() + i, NewClauses.begin() + j,
                         shorter_filter);
        MakeNewInstruction = true;
        break;
      }

    // Look for the next batch of filters.
    i = j + 1;
  }

  // If typeinfos matched if and only if equal, then the elements of a filter L
  // that occurs later than a filter F could be replaced by the intersection of
  // the elements of F and L.  In reality two typeinfos can match without being
  // equal (for example if one represents a C++ class, and the other some class
  // derived from it) so it would be wrong to perform this transform in general.
  // However the transform is correct and useful if F is a subset of L.  In that
  // case L can be replaced by F, and thus removed altogether since repeating a
  // filter is pointless.  So here we look at all pairs of filters F and L where
  // L follows F in the list of clauses, and remove L if every element of F is
  // an element of L.  This can occur when inlining C++ functions with exception
  // specifications.
  for (unsigned i = 0; i + 1 < NewClauses.size(); ++i) {
    // Examine each filter in turn.
    Value *Filter = NewClauses[i];
    ArrayType *FTy = dyn_cast<ArrayType>(Filter->getType());
    if (!FTy)
      // Not a filter - skip it.
      continue;
    unsigned FElts = FTy->getNumElements();
    // Examine each filter following this one.  Doing this backwards means that
    // we don't have to worry about filters disappearing under us when removed.
    for (unsigned j = NewClauses.size() - 1; j != i; --j) {
      Value *LFilter = NewClauses[j];
      ArrayType *LTy = dyn_cast<ArrayType>(LFilter->getType());
      if (!LTy)
        // Not a filter - skip it.
        continue;
      // If Filter is a subset of LFilter, i.e. every element of Filter is also
      // an element of LFilter, then discard LFilter.
      SmallVectorImpl<Value *>::iterator J = NewClauses.begin() + j;
      // If Filter is empty then it is a subset of LFilter.
      if (!FElts) {
        // Discard LFilter.
        NewClauses.erase(J);
        MakeNewInstruction = true;
        // Move on to the next filter.
        continue;
      }
      unsigned LElts = LTy->getNumElements();
      // If Filter is longer than LFilter then it cannot be a subset of it.
      if (FElts > LElts)
        // Move on to the next filter.
        continue;
      // At this point we know that LFilter has at least one element.
      if (isa<ConstantAggregateZero>(LFilter)) { // LFilter only contains zeros.
        // Filter is a subset of LFilter iff Filter contains only zeros (as we
        // already know that Filter is not longer than LFilter).
        if (isa<ConstantAggregateZero>(Filter)) {
          assert(FElts <= LElts && "Should have handled this case earlier!");
          // Discard LFilter.
          NewClauses.erase(J);
          MakeNewInstruction = true;
        }
        // Move on to the next filter.
        continue;
      }
      ConstantArray *LArray = cast<ConstantArray>(LFilter);
      if (isa<ConstantAggregateZero>(Filter)) { // Filter only contains zeros.
        // Since Filter is non-empty and contains only zeros, it is a subset of
        // LFilter iff LFilter contains a zero.
        assert(FElts > 0 && "Should have eliminated the empty filter earlier!");
        for (unsigned l = 0; l != LElts; ++l)
          if (LArray->getOperand(l)->isNullValue()) {
            // LFilter contains a zero - discard it.
            NewClauses.erase(J);
            MakeNewInstruction = true;
            break;
          }
        // Move on to the next filter.
        continue;
      }
      // At this point we know that both filters are ConstantArrays.  Loop over
      // operands to see whether every element of Filter is also an element of
      // LFilter.  Since filters tend to be short this is probably faster than
      // using a method that scales nicely.
      ConstantArray *FArray = cast<ConstantArray>(Filter);
      bool AllFound = true;
      for (unsigned f = 0; f != FElts; ++f) {
        Value *FTypeInfo = FArray->getOperand(f)->stripPointerCasts();
        AllFound = false;
        for (unsigned l = 0; l != LElts; ++l) {
          Value *LTypeInfo = LArray->getOperand(l)->stripPointerCasts();
          if (LTypeInfo == FTypeInfo) {
            AllFound = true;
            break;
          }
        }
        if (!AllFound)
          break;
      }
      if (AllFound) {
        // Discard LFilter.
        NewClauses.erase(J);
        MakeNewInstruction = true;
      }
      // Move on to the next filter.
    }
  }

  // If we changed any of the clauses, replace the old landingpad instruction
  // with a new one.
  if (MakeNewInstruction) {
    LandingPadInst *NLI = LandingPadInst::Create(LI.getType(),
                                                 LI.getPersonalityFn(),
                                                 NewClauses.size());
    for (unsigned i = 0, e = NewClauses.size(); i != e; ++i)
      NLI->addClause(NewClauses[i]);
    // A landing pad with no clauses must have the cleanup flag set.  It is
    // theoretically possible, though highly unlikely, that we eliminated all
    // clauses.  If so, force the cleanup flag to true.
    if (NewClauses.empty())
      CleanupFlag = true;
    NLI->setCleanup(CleanupFlag);
    return NLI;
  }

  // Even if none of the clauses changed, we may nonetheless have understood
  // that the cleanup flag is pointless.  Clear it if so.
  if (LI.isCleanup() != CleanupFlag) {
    assert(!CleanupFlag && "Adding a cleanup, not removing one?!");
    LI.setCleanup(CleanupFlag);
    return &LI;
  }

  return 0;
}




/// TryToSinkInstruction - Try to move the specified instruction from its
/// current block into the beginning of DestBlock, which can only happen if it's
/// safe to move the instruction past all of the instructions between it and the
/// end of its block.
static bool TryToSinkInstruction(Instruction *I, BasicBlock *DestBlock) {
  assert(I->hasOneUse() && "Invariants didn't hold!");

  // Cannot move control-flow-involving, volatile loads, vaarg, etc.
  if (isa<PHINode>(I) || isa<LandingPadInst>(I) || I->mayHaveSideEffects() ||
      isa<TerminatorInst>(I))
    return false;

  // Do not sink alloca instructions out of the entry block.
  if (isa<AllocaInst>(I) && I->getParent() ==
        &DestBlock->getParent()->getEntryBlock())
    return false;

  // We can only sink load instructions if there is nothing between the load and
  // the end of block that could change the value.
  if (I->mayReadFromMemory()) {
    for (BasicBlock::iterator Scan = I, E = I->getParent()->end();
         Scan != E; ++Scan)
      if (Scan->mayWriteToMemory())
        return false;
  }

  BasicBlock::iterator InsertPos = DestBlock->getFirstInsertionPt();
  I->moveBefore(InsertPos);
  ++NumSunkInst;
  return true;
}


/// AddReachableCodeToWorklist - Walk the function in depth-first order, adding
/// all reachable code to the worklist.
///
/// This has a couple of tricks to make the code faster and more powerful.  In
/// particular, we constant fold and DCE instructions as we go, to avoid adding
/// them to the worklist (this significantly speeds up instcombine on code where
/// many instructions are dead or constant).  Additionally, if we find a branch
/// whose condition is a known constant, we only visit the reachable successors.
///
static bool AddReachableCodeToWorklist(BasicBlock *BB,
                                       SmallPtrSet<BasicBlock*, 64> &Visited,
                                       InstCombiner &IC,
                                       const DataLayout *DL,
                                       const TargetLibraryInfo *TLI) {
  bool MadeIRChange = false;
  SmallVector<BasicBlock*, 256> Worklist;
  Worklist.push_back(BB);

  SmallVector<Instruction*, 128> InstrsForInstCombineWorklist;
  DenseMap<ConstantExpr*, Constant*> FoldedConstants;

  do {
    BB = Worklist.pop_back_val();

    // We have now visited this block!  If we've already been here, ignore it.
    if (!Visited.insert(BB)) continue;

    for (BasicBlock::iterator BBI = BB->begin(), E = BB->end(); BBI != E; ) {
      Instruction *Inst = BBI++;

      // DCE instruction if trivially dead.
      if (isInstructionTriviallyDead(Inst, TLI)) {
        ++NumDeadInst;
        DEBUG(dbgs() << "IC: DCE: " << *Inst << '\n');
        Inst->eraseFromParent();
        continue;
      }

      // ConstantProp instruction if trivially constant.
      if (!Inst->use_empty() && isa<Constant>(Inst->getOperand(0)))
        if (Constant *C = ConstantFoldInstruction(Inst, DL, TLI)) {
          DEBUG(dbgs() << "IC: ConstFold to: " << *C << " from: "
                       << *Inst << '\n');
          Inst->replaceAllUsesWith(C);
          ++NumConstProp;
          Inst->eraseFromParent();
          continue;
        }

      if (DL) {
        // See if we can constant fold its operands.
        for (User::op_iterator i = Inst->op_begin(), e = Inst->op_end();
             i != e; ++i) {
          ConstantExpr *CE = dyn_cast<ConstantExpr>(i);
          if (CE == 0) continue;

          Constant*& FoldRes = FoldedConstants[CE];
          if (!FoldRes)
            FoldRes = ConstantFoldConstantExpression(CE, DL, TLI);
          if (!FoldRes)
            FoldRes = CE;

          if (FoldRes != CE) {
            *i = FoldRes;
            MadeIRChange = true;
          }
        }
      }

      InstrsForInstCombineWorklist.push_back(Inst);
    }

    // Recursively visit successors.  If this is a branch or switch on a
    // constant, only visit the reachable successor.
    TerminatorInst *TI = BB->getTerminator();
    if (BranchInst *BI = dyn_cast<BranchInst>(TI)) {
      if (BI->isConditional() && isa<ConstantInt>(BI->getCondition())) {
        bool CondVal = cast<ConstantInt>(BI->getCondition())->getZExtValue();
        BasicBlock *ReachableBB = BI->getSuccessor(!CondVal);
        Worklist.push_back(ReachableBB);
        continue;
      }
    } else if (SwitchInst *SI = dyn_cast<SwitchInst>(TI)) {
      if (ConstantInt *Cond = dyn_cast<ConstantInt>(SI->getCondition())) {
        // See if this is an explicit destination.
        for (SwitchInst::CaseIt i = SI->case_begin(), e = SI->case_end();
             i != e; ++i)
          if (i.getCaseValue() == Cond) {
            BasicBlock *ReachableBB = i.getCaseSuccessor();
            Worklist.push_back(ReachableBB);
            continue;
          }

        // Otherwise it is the default destination.
        Worklist.push_back(SI->getDefaultDest());
        continue;
      }
    }

    for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i)
      Worklist.push_back(TI->getSuccessor(i));
  } while (!Worklist.empty());

  // Once we've found all of the instructions to add to instcombine's worklist,
  // add them in reverse order.  This way instcombine will visit from the top
  // of the function down.  This jives well with the way that it adds all uses
  // of instructions to the worklist after doing a transformation, thus avoiding
  // some N^2 behavior in pathological cases.
  IC.Worklist.AddInitialGroup(&InstrsForInstCombineWorklist[0],
                              InstrsForInstCombineWorklist.size());

  return MadeIRChange;
}

bool InstCombiner::DoOneIteration(Function &F, unsigned Iteration) {
  MadeIRChange = false;

  DEBUG(dbgs() << "\n\nINSTCOMBINE ITERATION #" << Iteration << " on "
               << F.getName() << "\n");

  {
    // Do a depth-first traversal of the function, populate the worklist with
    // the reachable instructions.  Ignore blocks that are not reachable.  Keep
    // track of which blocks we visit.
    SmallPtrSet<BasicBlock*, 64> Visited;
    MadeIRChange |= AddReachableCodeToWorklist(F.begin(), Visited, *this, DL,
                                               TLI);

    // Do a quick scan over the function.  If we find any blocks that are
    // unreachable, remove any instructions inside of them.  This prevents
    // the instcombine code from having to deal with some bad special cases.
    for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
      if (Visited.count(BB)) continue;

      // Delete the instructions backwards, as it has a reduced likelihood of
      // having to update as many def-use and use-def chains.
      Instruction *EndInst = BB->getTerminator(); // Last not to be deleted.
      while (EndInst != BB->begin()) {
        // Delete the next to last instruction.
        BasicBlock::iterator I = EndInst;
        Instruction *Inst = --I;
        if (!Inst->use_empty())
          Inst->replaceAllUsesWith(UndefValue::get(Inst->getType()));
        if (isa<LandingPadInst>(Inst)) {
          EndInst = Inst;
          continue;
        }
        if (!isa<DbgInfoIntrinsic>(Inst)) {
          ++NumDeadInst;
          MadeIRChange = true;
        }
        Inst->eraseFromParent();
      }
    }
  }

  while (!Worklist.isEmpty()) {
    Instruction *I = Worklist.RemoveOne();
    if (I == 0) continue;  // skip null values.

    // Check to see if we can DCE the instruction.
    if (isInstructionTriviallyDead(I, TLI)) {
      DEBUG(dbgs() << "IC: DCE: " << *I << '\n');
      EraseInstFromFunction(*I);
      ++NumDeadInst;
      MadeIRChange = true;
      continue;
    }

    // Instruction isn't dead, see if we can constant propagate it.
    if (!I->use_empty() && isa<Constant>(I->getOperand(0)))
      if (Constant *C = ConstantFoldInstruction(I, DL, TLI)) {
        DEBUG(dbgs() << "IC: ConstFold to: " << *C << " from: " << *I << '\n');

        // Add operands to the worklist.
        ReplaceInstUsesWith(*I, C);
        ++NumConstProp;
        EraseInstFromFunction(*I);
        MadeIRChange = true;
        continue;
      }

    // See if we can trivially sink this instruction to a successor basic block.
    if (I->hasOneUse()) {
      BasicBlock *BB = I->getParent();
      Instruction *UserInst = cast<Instruction>(I->use_back());
      BasicBlock *UserParent;

      // Get the block the use occurs in.
      if (PHINode *PN = dyn_cast<PHINode>(UserInst))
        UserParent = PN->getIncomingBlock(I->use_begin().getUse());
      else
        UserParent = UserInst->getParent();

      if (UserParent != BB) {
        bool UserIsSuccessor = false;
        // See if the user is one of our successors.
        for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI)
          if (*SI == UserParent) {
            UserIsSuccessor = true;
            break;
          }

        // If the user is one of our immediate successors, and if that successor
        // only has us as a predecessors (we'd have to split the critical edge
        // otherwise), we can keep going.
        if (UserIsSuccessor && UserParent->getSinglePredecessor())
          // Okay, the CFG is simple enough, try to sink this instruction.
          MadeIRChange |= TryToSinkInstruction(I, UserParent);
      }
    }

    // Now that we have an instruction, try combining it to simplify it.
    Builder->SetInsertPoint(I->getParent(), I);
    Builder->SetCurrentDebugLocation(I->getDebugLoc());

#ifndef NDEBUG
    std::string OrigI;
#endif
    DEBUG(raw_string_ostream SS(OrigI); I->print(SS); OrigI = SS.str(););
    DEBUG(dbgs() << "IC: Visiting: " << OrigI << '\n');

    if (Instruction *Result = visit(*I)) {
      ++NumCombined;
      // Should we replace the old instruction with a new one?
      if (Result != I) {
        DEBUG(dbgs() << "IC: Old = " << *I << '\n'
                     << "    New = " << *Result << '\n');

        if (!I->getDebugLoc().isUnknown())
          Result->setDebugLoc(I->getDebugLoc());
        // Everything uses the new instruction now.
        I->replaceAllUsesWith(Result);

        // Move the name to the new instruction first.
        Result->takeName(I);

        // Push the new instruction and any users onto the worklist.
        Worklist.Add(Result);
        Worklist.AddUsersToWorkList(*Result);

        // Insert the new instruction into the basic block...
        BasicBlock *InstParent = I->getParent();
        BasicBlock::iterator InsertPos = I;

        // If we replace a PHI with something that isn't a PHI, fix up the
        // insertion point.
        if (!isa<PHINode>(Result) && isa<PHINode>(InsertPos))
          InsertPos = InstParent->getFirstInsertionPt();

        InstParent->getInstList().insert(InsertPos, Result);

        EraseInstFromFunction(*I);
      } else {
#ifndef NDEBUG
        DEBUG(dbgs() << "IC: Mod = " << OrigI << '\n'
                     << "    New = " << *I << '\n');
#endif

        // If the instruction was modified, it's possible that it is now dead.
        // if so, remove it.
        if (isInstructionTriviallyDead(I, TLI)) {
          EraseInstFromFunction(*I);
        } else {
          Worklist.Add(I);
          Worklist.AddUsersToWorkList(*I);
        }
      }
      MadeIRChange = true;
    }
  }

  Worklist.Zap();
  return MadeIRChange;
}

namespace {
class InstCombinerLibCallSimplifier : public LibCallSimplifier {
  InstCombiner *IC;
public:
  InstCombinerLibCallSimplifier(const DataLayout *DL,
                                const TargetLibraryInfo *TLI,
                                InstCombiner *IC)
    : LibCallSimplifier(DL, TLI, UnsafeFPShrink) {
    this->IC = IC;
  }

  /// replaceAllUsesWith - override so that instruction replacement
  /// can be defined in terms of the instruction combiner framework.
  virtual void replaceAllUsesWith(Instruction *I, Value *With) const {
    IC->ReplaceInstUsesWith(*I, With);
  }
};
}

bool InstCombiner::runOnFunction(Function &F) {
  if (skipOptnoneFunction(F))
    return false;

  DataLayoutPass *DLP = getAnalysisIfAvailable<DataLayoutPass>();
  DL = DLP ? &DLP->getDataLayout() : 0;
  TLI = &getAnalysis<TargetLibraryInfo>();
  // Minimizing size?
  MinimizeSize = F.getAttributes().hasAttribute(AttributeSet::FunctionIndex,
                                                Attribute::MinSize);

  /// Builder - This is an IRBuilder that automatically inserts new
  /// instructions into the worklist when they are created.
  IRBuilder<true, TargetFolder, InstCombineIRInserter>
    TheBuilder(F.getContext(), TargetFolder(DL),
               InstCombineIRInserter(Worklist));
  Builder = &TheBuilder;

  InstCombinerLibCallSimplifier TheSimplifier(DL, TLI, this);
  Simplifier = &TheSimplifier;

  bool EverMadeChange = false;

  // Lower dbg.declare intrinsics otherwise their value may be clobbered
  // by instcombiner.
  EverMadeChange = LowerDbgDeclare(F);

  // Iterate while there is work to do.
  unsigned Iteration = 0;
  while (DoOneIteration(F, Iteration++))
    EverMadeChange = true;

  Builder = 0;
  return EverMadeChange;
}

FunctionPass *llvm::createInstructionCombiningPass() {
  return new InstCombiner();
}
