//===- TraceValues.cpp - Value Tracing for debugging -------------*- C++ -*--=//
//
// Support for inserting LLVM code to print values at basic block and function
// exits.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation/TraceValues.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/iMemory.h"
#include "llvm/iTerminators.h"
#include "llvm/iOther.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Assembly/Writer.h"
#include "Support/CommandLine.h"
#include "Support/StringExtras.h"
#include <algorithm>
#include <sstream>
using std::vector;
using std::string;

static cl::opt<bool>
DisablePtrHashing("tracedisablehashdisable", cl::Hidden,
                  cl::desc("Disable pointer hashing"));

static cl::list<string>
TraceFuncName("tracefunc", cl::desc("trace only specific functions"),
              cl::value_desc("function"), cl::Hidden);

static void TraceValuesAtBBExit(BasicBlock *BB,
                                Function *Printf, Function* HashPtrToSeqNum,
                                vector<Instruction*> *valuesStoredInFunction);

// We trace a particular function if no functions to trace were specified
// or if the function is in the specified list.
// 
inline static bool
TraceThisFunction(Function &func)
{
  if (TraceFuncName.size() == 0)
    return true;

  return std::find(TraceFuncName.begin(), TraceFuncName.end(), func.getName())
                  != TraceFuncName.end();
}


namespace {
  struct ExternalFuncs {
    Function *PrintfFunc, *HashPtrFunc, *ReleasePtrFunc;
    Function *RecordPtrFunc, *PushOnEntryFunc, *ReleaseOnReturnFunc;
    void doInitialization(Module &M); // Add prototypes for external functions
  };
  
  class InsertTraceCode : public FunctionPass {
  protected:
    ExternalFuncs externalFuncs;
  public:
    
    // Add a prototype for runtime functions not already in the program.
    //
    bool doInitialization(Module &M);
    
    //--------------------------------------------------------------------------
    // Function InsertCodeToTraceValues
    // 
    // Inserts tracing code for all live values at basic block and/or function
    // exits as specified by `traceBasicBlockExits' and `traceFunctionExits'.
    //
    bool doit(Function *M);

    virtual void handleBasicBlock(BasicBlock *BB, vector<Instruction*> &VI) = 0;

    // runOnFunction - This method does the work.
    //
    bool runOnFunction(Function &F);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesCFG();
    }
  };

  struct FunctionTracer : public InsertTraceCode {
    // Ignore basic blocks here...
    virtual void handleBasicBlock(BasicBlock *BB, vector<Instruction*> &VI) {}
  };

  struct BasicBlockTracer : public InsertTraceCode {
    // Trace basic blocks here...
    virtual void handleBasicBlock(BasicBlock *BB, vector<Instruction*> &VI) {
      TraceValuesAtBBExit(BB, externalFuncs.PrintfFunc,
                          externalFuncs.HashPtrFunc, &VI);
    }
  };

  // Register the passes...
  RegisterOpt<FunctionTracer>  X("tracem","Insert Function trace code only");
  RegisterOpt<BasicBlockTracer> Y("trace","Insert BB and Function trace code");
} // end anonymous namespace


Pass *createTraceValuesPassForFunction() {     // Just trace functions
  return new FunctionTracer();
}

Pass *createTraceValuesPassForBasicBlocks() {  // Trace BB's and functions
  return new BasicBlockTracer();
}


// Add a prototype for external functions used by the tracing code.
//
void ExternalFuncs::doInitialization(Module &M) {
  const Type *SBP = PointerType::get(Type::SByteTy);
  const FunctionType *MTy =
    FunctionType::get(Type::IntTy, vector<const Type*>(1, SBP), true);
  PrintfFunc = M.getOrInsertFunction("printf", MTy);

  // uint (sbyte*)
  const FunctionType *hashFuncTy =
    FunctionType::get(Type::UIntTy, vector<const Type*>(1, SBP), false);
  HashPtrFunc = M.getOrInsertFunction("HashPointerToSeqNum", hashFuncTy);
  
  // void (sbyte*)
  const FunctionType *voidSBPFuncTy =
    FunctionType::get(Type::VoidTy, vector<const Type*>(1, SBP), false);
  
  ReleasePtrFunc = M.getOrInsertFunction("ReleasePointerSeqNum", voidSBPFuncTy);
  RecordPtrFunc  = M.getOrInsertFunction("RecordPointer", voidSBPFuncTy);
  
  const FunctionType *voidvoidFuncTy =
    FunctionType::get(Type::VoidTy, vector<const Type*>(), false);
  
  PushOnEntryFunc = M.getOrInsertFunction("PushPointerSet", voidvoidFuncTy);
  ReleaseOnReturnFunc = M.getOrInsertFunction("ReleasePointersPopSet",
                                               voidvoidFuncTy);
}


// Add a prototype for external functions used by the tracing code.
//
bool InsertTraceCode::doInitialization(Module &M) {
  externalFuncs.doInitialization(M);
  return false;
}


static inline GlobalVariable *getStringRef(Module *M, const string &str) {
  // Create a constant internal string reference...
  Constant *Init = ConstantArray::get(str);

  // Create the global variable and record it in the module
  // The GV will be renamed to a unique name if needed.
  GlobalVariable *GV = new GlobalVariable(Init->getType(), true, true, Init,
                                          "trstr");
  M->getGlobalList().push_back(GV);
  return GV;
}


// 
// Check if this instruction has any uses outside its basic block,
// or if it used by either a Call or Return instruction.
// 
static inline bool LiveAtBBExit(const Instruction* I) {
  const BasicBlock *BB = I->getParent();
  for (Value::use_const_iterator U = I->use_begin(); U != I->use_end(); ++U)
    if (const Instruction *UI = dyn_cast<Instruction>(*U))
      if (UI->getParent() != BB || isa<ReturnInst>(UI))
        return true;

  return false;
}


static inline bool TraceThisOpCode(unsigned opCode) {
  // Explicitly test for opCodes *not* to trace so that any new opcodes will
  // be traced by default (VoidTy's are already excluded)
  // 
  return (opCode  < Instruction::OtherOpsBegin &&
          opCode != Instruction::Alloca &&
          opCode != Instruction::PHINode &&
          opCode != Instruction::Cast);
}


static bool ShouldTraceValue(const Instruction *I) {
  return
    I->getType() != Type::VoidTy && LiveAtBBExit(I) &&
    TraceThisOpCode(I->getOpcode());
}

static string getPrintfCodeFor(const Value *V) {
  if (V == 0) return "";
  if (V->getType()->isFloatingPoint())
    return "%g";
  else if (V->getType() == Type::LabelTy)
    return "0x%p";
  else if (isa<PointerType>(V->getType()))
    return DisablePtrHashing ? "0x%p" : "%d";
  else if (V->getType()->isIntegral())
    return "%d";
  
  assert(0 && "Illegal value to print out...");
  return "";
}


static void InsertPrintInst(Value *V, BasicBlock *BB, Instruction *InsertBefore,
                            string Message,
                            Function *Printf, Function* HashPtrToSeqNum) {
  // Escape Message by replacing all % characters with %% chars.
  string Tmp;
  std::swap(Tmp, Message);
  string::iterator I = std::find(Tmp.begin(), Tmp.end(), '%');
  while (I != Tmp.end()) {
    Message.append(Tmp.begin(), I);
    Message += "%%";
    ++I; // Make sure to erase the % as well...
    Tmp.erase(Tmp.begin(), I);
    I = std::find(Tmp.begin(), Tmp.end(), '%');
  }

  Module *Mod = BB->getParent()->getParent();

  // Turn the marker string into a global variable...
  GlobalVariable *fmtVal = getStringRef(Mod, Message+getPrintfCodeFor(V)+"\n");

  // Turn the format string into an sbyte *
  Instruction *GEP = 
    new GetElementPtrInst(fmtVal,
                          vector<Value*>(2,ConstantSInt::get(Type::LongTy, 0)),
                          "trstr", InsertBefore);
  
  // Insert a call to the hash function if this is a pointer value
  if (V && isa<PointerType>(V->getType()) && !DisablePtrHashing) {
    const Type *SBP = PointerType::get(Type::SByteTy);
    if (V->getType() != SBP)     // Cast pointer to be sbyte*
      V = new CastInst(V, SBP, "Hash_cast", InsertBefore);

    vector<Value*> HashArgs(1, V);
    V = new CallInst(HashPtrToSeqNum, HashArgs, "ptrSeqNum", InsertBefore);
  }
  
  // Insert the first print instruction to print the string flag:
  vector<Value*> PrintArgs;
  PrintArgs.push_back(GEP);
  if (V) PrintArgs.push_back(V);
  new CallInst(Printf, PrintArgs, "trace", InsertBefore);
}
                            

static void InsertVerbosePrintInst(Value *V, BasicBlock *BB,
                                   Instruction *InsertBefore,
                                   const string &Message, Function *Printf,
                                   Function* HashPtrToSeqNum) {
  std::ostringstream OutStr;
  if (V) WriteAsOperand(OutStr, V);
  InsertPrintInst(V, BB, InsertBefore, Message+OutStr.str()+" = ",
                  Printf, HashPtrToSeqNum);
}

static void 
InsertReleaseInst(Value *V, BasicBlock *BB,
                  Instruction *InsertBefore,
                  Function* ReleasePtrFunc) {
  
  const Type *SBP = PointerType::get(Type::SByteTy);
  if (V->getType() != SBP)    // Cast pointer to be sbyte*
    V = new CastInst(V, SBP, "RPSN_cast", InsertBefore);

  vector<Value*> releaseArgs(1, V);
  new CallInst(ReleasePtrFunc, releaseArgs, "", InsertBefore);
}

static void 
InsertRecordInst(Value *V, BasicBlock *BB,
                 Instruction *InsertBefore,
                 Function* RecordPtrFunc) {
    const Type *SBP = PointerType::get(Type::SByteTy);
  if (V->getType() != SBP)     // Cast pointer to be sbyte*
    V = new CastInst(V, SBP, "RP_cast", InsertBefore);

  vector<Value*> releaseArgs(1, V);
  new CallInst(RecordPtrFunc, releaseArgs, "", InsertBefore);
}

// Look for alloca and free instructions. These are the ptrs to release.
// Release the free'd pointers immediately.  Record the alloca'd pointers
// to be released on return from the current function.
// 
static void
ReleasePtrSeqNumbers(BasicBlock *BB,
                     ExternalFuncs& externalFuncs) {
  
  for (BasicBlock::iterator II=BB->begin(), IE = BB->end(); II != IE; ++II)
    if (FreeInst *FI = dyn_cast<FreeInst>(&*II))
      InsertReleaseInst(FI->getOperand(0), BB, FI,externalFuncs.ReleasePtrFunc);
    else if (AllocaInst *AI = dyn_cast<AllocaInst>(&*II))
      InsertRecordInst(AI, BB, AI->getNext(), externalFuncs.RecordPtrFunc);
}  


// Insert print instructions at the end of basic block BB for each value
// computed in BB that is live at the end of BB,
// or that is stored to memory in BB.
// If the value is stored to memory, we load it back before printing it
// We also return all such loaded values in the vector valuesStoredInFunction
// for printing at the exit from the function.  (Note that in each invocation
// of the function, this will only get the last value stored for each static
// store instruction).
// 
static void TraceValuesAtBBExit(BasicBlock *BB,
                                Function *Printf, Function* HashPtrToSeqNum,
                                vector<Instruction*> *valuesStoredInFunction) {
  // Get an iterator to point to the insertion location, which is
  // just before the terminator instruction.
  // 
  TerminatorInst *InsertPos = BB->getTerminator();
  
  std::ostringstream OutStr;
  WriteAsOperand(OutStr, BB, false);
  InsertPrintInst(0, BB, InsertPos, "LEAVING BB:" + OutStr.str(),
                  Printf, HashPtrToSeqNum);

  // Insert a print instruction for each instruction preceding InsertPos.
  // The print instructions must go before InsertPos, so we use the
  // instruction *preceding* InsertPos to check when to terminate the loop.
  // 
  for (BasicBlock::iterator II = BB->begin(); &*II != InsertPos; ++II) {
    if (StoreInst *SI = dyn_cast<StoreInst>(&*II)) {
      assert(valuesStoredInFunction &&
             "Should not be printing a store instruction at function exit");
      LoadInst *LI = new LoadInst(SI->getPointerOperand(), "reload." +
                                  SI->getPointerOperand()->getName(),
                                  InsertPos);
      valuesStoredInFunction->push_back(LI);
    }
    if (ShouldTraceValue(II))
      InsertVerbosePrintInst(II, BB, InsertPos, "  ", Printf, HashPtrToSeqNum);
  }
}

static inline void InsertCodeToShowFunctionEntry(Function &F, Function *Printf,
                                                 Function* HashPtrToSeqNum){
  // Get an iterator to point to the insertion location
  BasicBlock &BB = F.getEntryNode();
  Instruction *InsertPos = BB.begin();

  std::ostringstream OutStr;
  WriteAsOperand(OutStr, &F, true);
  InsertPrintInst(0, &BB, InsertPos, "ENTERING FUNCTION: " + OutStr.str(),
                  Printf, HashPtrToSeqNum);

  // Now print all the incoming arguments
  unsigned ArgNo = 0;
  for (Function::aiterator I = F.abegin(), E = F.aend(); I != E; ++I, ++ArgNo){
    InsertVerbosePrintInst(I, &BB, InsertPos,
                           "  Arg #" + utostr(ArgNo) + ": ", Printf,
                           HashPtrToSeqNum);
  }
}


static inline void InsertCodeToShowFunctionExit(BasicBlock *BB,
                                                Function *Printf,
                                                Function* HashPtrToSeqNum) {
  // Get an iterator to point to the insertion location
  ReturnInst *Ret = cast<ReturnInst>(BB->getTerminator());
  
  std::ostringstream OutStr;
  WriteAsOperand(OutStr, BB->getParent(), true);
  InsertPrintInst(0, BB, Ret, "LEAVING  FUNCTION: " + OutStr.str(),
                  Printf, HashPtrToSeqNum);
  
  // print the return value, if any
  if (BB->getParent()->getReturnType() != Type::VoidTy)
    InsertPrintInst(Ret->getReturnValue(), BB, Ret, "  Returning: ",
                    Printf, HashPtrToSeqNum);
}


bool InsertTraceCode::runOnFunction(Function &F) {
  if (!TraceThisFunction(F))
    return false;
  
  vector<Instruction*> valuesStoredInFunction;
  vector<BasicBlock*>  exitBlocks;

  // Insert code to trace values at function entry
  InsertCodeToShowFunctionEntry(F, externalFuncs.PrintfFunc,
                                externalFuncs.HashPtrFunc);
  
  // Push a pointer set for recording alloca'd pointers at entry.
  if (!DisablePtrHashing)
    new CallInst(externalFuncs.PushOnEntryFunc, vector<Value*>(), "",
                 F.getEntryNode().begin());

  for (Function::iterator BB = F.begin(); BB != F.end(); ++BB) {
    if (isa<ReturnInst>(BB->getTerminator()))
      exitBlocks.push_back(BB); // record this as an exit block

    // Insert trace code if this basic block is interesting...
    handleBasicBlock(BB, valuesStoredInFunction);

    if (!DisablePtrHashing)          // release seq. numbers on free/ret
      ReleasePtrSeqNumbers(BB, externalFuncs);
  }
  
  for (unsigned i=0; i != exitBlocks.size(); ++i)
    {
      // Insert code to trace values at function exit
      InsertCodeToShowFunctionExit(exitBlocks[i], externalFuncs.PrintfFunc,
                                   externalFuncs.HashPtrFunc);
      
      // Release all recorded pointers before RETURN.  Do this LAST!
      if (!DisablePtrHashing)
        new CallInst(externalFuncs.ReleaseOnReturnFunc, vector<Value*>(), "",
                     exitBlocks[i]->getTerminator());
    }
  
  return true;
}
