//===- IndVarSimplify.cpp - Induction Variable Elimination ----------------===//
//
// InductionVariableSimplify - Transform induction variables in a program
//   to all use a single cannonical induction variable per loop.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/InductionVariable.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Writer.h"
#include "llvm/iPHINode.h"
#include "llvm/iOther.h"
#include "llvm/Type.h"
#include "llvm/Constants.h"
#include "llvm/Support/CFG.h"
#include "Support/STLExtras.h"
#include "Support/StatisticReporter.h"

static Statistic<> NumRemoved ("indvars\t\t- Number of aux indvars removed");
static Statistic<> NumInserted("indvars\t\t- Number of cannonical indvars added");


// InsertCast - Cast Val to Ty, setting a useful name on the cast if Val has a
// name...
//
static Instruction *InsertCast(Instruction *Val, const Type *Ty,
                               BasicBlock::iterator It) {
  Instruction *Cast = new CastInst(Val, Ty);
  if (Val->hasName()) Cast->setName(Val->getName()+"-casted");
  Val->getParent()->getInstList().insert(It, Cast);
  return Cast;
}

static bool TransformLoop(LoopInfo *Loops, Loop *Loop) {
  // Transform all subloops before this loop...
  bool Changed = reduce_apply_bool(Loop->getSubLoops().begin(),
                                   Loop->getSubLoops().end(),
                              std::bind1st(std::ptr_fun(TransformLoop), Loops));
  // Get the header node for this loop.  All of the phi nodes that could be
  // induction variables must live in this basic block.
  //
  BasicBlock *Header = Loop->getBlocks().front();
  
  // Loop over all of the PHI nodes in the basic block, calculating the
  // induction variables that they represent... stuffing the induction variable
  // info into a vector...
  //
  std::vector<InductionVariable> IndVars;    // Induction variables for block
  BasicBlock::iterator AfterPHIIt = Header->begin();
  for (; PHINode *PN = dyn_cast<PHINode>(&*AfterPHIIt); ++AfterPHIIt)
    IndVars.push_back(InductionVariable(PN, Loops));
  // AfterPHIIt now points to first nonphi instruction...

  // If there are no phi nodes in this basic block, there can't be indvars...
  if (IndVars.empty()) return Changed;
  
  // Loop over the induction variables, looking for a cannonical induction
  // variable, and checking to make sure they are not all unknown induction
  // variables.
  //
  bool FoundIndVars = false;
  InductionVariable *Cannonical = 0;
  for (unsigned i = 0; i < IndVars.size(); ++i) {
    if (IndVars[i].InductionType == InductionVariable::Cannonical)
      Cannonical = &IndVars[i];
    if (IndVars[i].InductionType != InductionVariable::Unknown)
      FoundIndVars = true;
  }

  // No induction variables, bail early... don't add a cannonnical indvar
  if (!FoundIndVars) return Changed;

  // Okay, we want to convert other induction variables to use a cannonical
  // indvar.  If we don't have one, add one now...
  if (!Cannonical) {
    // Create the PHI node for the new induction variable
    PHINode *PN = new PHINode(Type::UIntTy, "cann-indvar");

    // Insert the phi node at the end of the other phi nodes...
    AfterPHIIt = ++Header->getInstList().insert(AfterPHIIt, PN);

    // Create the increment instruction to add one to the counter...
    Instruction *Add = BinaryOperator::create(Instruction::Add, PN,
                                              ConstantUInt::get(Type::UIntTy,1),
                                              "add1-indvar");

    // Insert the add instruction after all of the PHI nodes...
    Header->getInstList().insert(AfterPHIIt, Add);

    // Figure out which block is incoming and which is the backedge for the loop
    BasicBlock *Incoming, *BackEdgeBlock;
    pred_iterator PI = pred_begin(Header);
    assert(PI != pred_end(Header) && "Loop headers should have 2 preds!");
    if (Loop->contains(*PI)) {  // First pred is back edge...
      BackEdgeBlock = *PI++;
      Incoming      = *PI++;
    } else {
      Incoming      = *PI++;
      BackEdgeBlock = *PI++;
    }
    assert(PI == pred_end(Header) && "Loop headers should have 2 preds!");
    
    // Add incoming values for the PHI node...
    PN->addIncoming(Constant::getNullValue(Type::UIntTy), Incoming);
    PN->addIncoming(Add, BackEdgeBlock);

    // Analyze the new induction variable...
    IndVars.push_back(InductionVariable(PN, Loops));
    assert(IndVars.back().InductionType == InductionVariable::Cannonical &&
           "Just inserted cannonical indvar that is not cannonical!");
    Cannonical = &IndVars.back();
    ++NumInserted;
    Changed = true;
  }

  DEBUG(cerr << "Induction variables:\n");

  // Get the current loop iteration count, which is always the value of the
  // cannonical phi node...
  //
  PHINode *IterCount = Cannonical->Phi;

  // Loop through and replace all of the auxillary induction variables with
  // references to the primary induction variable...
  //
  for (unsigned i = 0; i < IndVars.size(); ++i) {
    InductionVariable *IV = &IndVars[i];

    DEBUG(cerr << IV);

    // Don't modify the cannonical indvar or unrecognized indvars...
    if (IV != Cannonical && IV->InductionType != InductionVariable::Unknown) {
      Instruction *Val = IterCount;
      if (!isa<ConstantInt>(IV->Step) ||   // If the step != 1
          !cast<ConstantInt>(IV->Step)->equalsInt(1)) {
        std::string Name;   // Create a scale by the step value...
        if (IV->Phi->hasName()) Name = IV->Phi->getName()+"-scale";

        // If the types are not compatible, insert a cast now...
        if (Val->getType() != IV->Step->getType())
          Val = InsertCast(Val, IV->Step->getType(), AfterPHIIt);

        Val = BinaryOperator::create(Instruction::Mul, Val, IV->Step, Name);
        // Insert the phi node at the end of the other phi nodes...
        Header->getInstList().insert(AfterPHIIt, Val);
      }

      if (!isa<Constant>(IV->Start) ||   // If the start != 0
          !cast<Constant>(IV->Start)->isNullValue()) {
        std::string Name;   // Create a offset by the start value...
        if (IV->Phi->hasName()) Name = IV->Phi->getName()+"-offset";

        // If the types are not compatible, insert a cast now...
        if (Val->getType() != IV->Start->getType())
          Val = InsertCast(Val, IV->Start->getType(), AfterPHIIt);

        Val = BinaryOperator::create(Instruction::Add, Val, IV->Start, Name);
        // Insert the phi node at the end of the other phi nodes...
        Header->getInstList().insert(AfterPHIIt, Val);
      }

      // If the PHI node has a different type than val is, insert a cast now...
      if (Val->getType() != IV->Phi->getType())
        Val = InsertCast(Val, IV->Phi->getType(), AfterPHIIt);
      
      // Replace all uses of the old PHI node with the new computed value...
      IV->Phi->replaceAllUsesWith(Val);

      // Move the PHI name to it's new equivalent value...
      std::string OldName = IV->Phi->getName();
      IV->Phi->setName("");
      Val->setName(OldName);

      // Delete the old, now unused, phi node...
      Header->getInstList().erase(IV->Phi);
      Changed = true;
      ++NumRemoved;
    }
  }

  return Changed;
}

namespace {
  struct InductionVariableSimplify : public FunctionPass {
    const char *getPassName() const {
      return "Induction Variable Cannonicalize";
    }

    virtual bool runOnFunction(Function &) {
      LoopInfo &LI = getAnalysis<LoopInfo>();

      // Induction Variables live in the header nodes of loops
      return reduce_apply_bool(LI.getTopLevelLoops().begin(),
                               LI.getTopLevelLoops().end(),
                               std::bind1st(std::ptr_fun(TransformLoop), &LI));
    }
    
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired(LoopInfo::ID);
      AU.preservesCFG();
    }
  };
}

Pass *createIndVarSimplifyPass() {
  return new InductionVariableSimplify();
}

