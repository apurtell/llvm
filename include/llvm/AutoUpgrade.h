//===-- llvm/AutoUpgrade.h - AutoUpgrade Helpers ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  These functions are implemented by lib/IR/AutoUpgrade.cpp.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_AUTOUPGRADE_H
#define LLVM_AUTOUPGRADE_H

namespace llvm {
  class Module;
  class GlobalVariable;
  class Function;
  class CallInst;
  class Instruction;

  /// This is a more granular function that simply checks an intrinsic function
  /// for upgrading, and returns true if it requires upgrading. It may return
  /// null in NewFn if the all calls to the original intrinsic function
  /// should be transformed to non-function-call instructions.
  bool UpgradeIntrinsicFunction(Function *F, Function *&NewFn);

  /// This is the complement to the above, replacing a specific call to an
  /// intrinsic function with a call to the specified new function.
  void UpgradeIntrinsicCall(CallInst *CI, Function *NewFn);

  /// This is an auto-upgrade hook for any old intrinsic function syntaxes
  /// which need to have both the function updated as well as all calls updated
  /// to the new function. This should only be run in a post-processing fashion
  /// so that it can update all calls to the old function.
  void UpgradeCallsToIntrinsic(Function* F);

  /// This checks for global variables which should be upgraded. It returns true
  /// if it requires upgrading.
  bool UpgradeGlobalVariable(GlobalVariable *GV);

  /// If the TBAA tag for the given instruction uses the scalar TBAA format,
  /// we upgrade it to the struct-path aware TBAA format.
  void UpgradeInstWithTBAATag(Instruction *I);
} // End llvm namespace

#endif
