//===- ObjCARCAliasAnalysis.h - ObjC ARC Optimization -*- C++ -*-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file declares a simple ARC-aware AliasAnalysis using special knowledge
/// of Objective C to enhance other optimization passes which rely on the Alias
/// Analysis infrastructure.
///
/// WARNING: This file knows about certain library functions. It recognizes them
/// by name, and hardwires knowledge of their semantics.
///
/// WARNING: This file knows about how certain Objective-C library functions are
/// used. Naive LLVM IR transformations which would otherwise be
/// behavior-preserving may break these assumptions.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_OBJCARC_OBJCARCALIASANALYSIS_H
#define LLVM_TRANSFORMS_OBJCARC_OBJCARCALIASANALYSIS_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Pass.h"

namespace llvm {
namespace objcarc {

  /// \brief This is a simple alias analysis implementation that uses knowledge
  /// of ARC constructs to answer queries.
  ///
  /// TODO: This class could be generalized to know about other ObjC-specific
  /// tricks. Such as knowing that ivars in the non-fragile ABI are non-aliasing
  /// even though their offsets are dynamic.
  class LLVM_LIBRARY_VISIBILITY ObjCARCAliasAnalysis : public ImmutablePass,
                                                       public AliasAnalysis {
  public:
    static char ID; // Class identification, replacement for typeinfo
    ObjCARCAliasAnalysis() : ImmutablePass(ID) {
      initializeObjCARCAliasAnalysisPass(*PassRegistry::getPassRegistry());
    }

  private:
    virtual void initializePass() {
      InitializeAliasAnalysis(this);
    }

    /// This method is used when a pass implements an analysis interface through
    /// multiple inheritance.  If needed, it should override this to adjust the
    /// this pointer as needed for the specified pass info.
    virtual void *getAdjustedAnalysisPointer(const void *PI) {
      if (PI == &AliasAnalysis::ID)
        return static_cast<AliasAnalysis *>(this);
      return this;
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual AliasResult alias(const Location &LocA, const Location &LocB);
    virtual bool pointsToConstantMemory(const Location &Loc, bool OrLocal);
    virtual ModRefBehavior getModRefBehavior(ImmutableCallSite CS);
    virtual ModRefBehavior getModRefBehavior(const Function *F);
    virtual ModRefResult getModRefInfo(ImmutableCallSite CS,
                                       const Location &Loc);
    virtual ModRefResult getModRefInfo(ImmutableCallSite CS1,
                                       ImmutableCallSite CS2);
  };

} // namespace objcarc
} // namespace llvm

#endif // LLVM_TRANSFORMS_OBJCARC_OBJCARCALIASANALYSIS_H
