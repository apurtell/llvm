//===- llvm/Analysis/InductionVariable.h - Induction variable ----*- C++ -*--=//
//
// This interface is used to identify and classify induction variables that
// exist in the program.  Induction variables must contain a PHI node that
// exists in a loop header.  Because of this, they are identified an managed by
// this PHI node.
//
// Induction variables are classified into a type.  Knowing that an induction
// variable is of a specific type can constrain the values of the start and
// step.  For example, a SimpleLinear induction variable must have a start and
// step values that are constants.
//
// Induction variables can be created with or without loop information.  If no
// loop information is available, induction variables cannot be recognized to be
// more than SimpleLinear variables.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_INDUCTIONVARIABLE_H
#define LLVM_ANALYSIS_INDUCTIONVARIABLE_H

class Value;
class PHINode;
class Instruction;
namespace cfg { class LoopInfo; class Loop; }

class InductionVariable {
public:
  enum iType {               // Identify the type of this induction variable
    Cannonical,              // Starts at 0, counts by 1
    SimpleLinear,            // Simple linear: Constant start, constant step
    Linear,                  // General linear:  loop invariant start, and step
    Unknown,                 // Unknown type.  Start & Step are null
  } InductionType;
  
  Value *Start, *Step;       // Start and step expressions for this indvar
  PHINode *Phi;              // The PHI node that corresponds to this indvar
public:

  // Create an induction variable for the specified value.  If it is a PHI, and
  // if it's recognizable, classify it and fill in instance variables.
  //
  InductionVariable(Instruction *V, cfg::LoopInfo *LoopInfo = 0);


  // Classify Induction
  static enum iType Classify(const Value *Start, const Value *Step,
			     const cfg::Loop *L = 0);
					      

};

#endif
