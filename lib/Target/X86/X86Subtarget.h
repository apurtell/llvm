//=====---- X86Subtarget.h - Define Subtarget for the X86 -----*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by Nate Begeman and is distributed under the
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the X86 specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef X86SUBTARGET_H
#define X86SUBTARGET_H

#include "llvm/Target/TargetSubtarget.h"

#include <string>

namespace llvm {
class Module;

class X86Subtarget : public TargetSubtarget {
protected:
  /// stackAlignment - The minimum alignment known to hold of the stack frame on
  /// entry to the function and which must be maintained by every function.
  unsigned stackAlignment;

  /// Used by instruction selector
  bool indirectExternAndWeakGlobals;

  /// Arch. features used by isel.
  bool Is64Bit;
  bool HasMMX;
  bool HasSSE;
  bool HasSSE2;
  bool HasSSE3;
  bool Has3DNow;
  bool Has3DNowA;
public:
  enum {
    isELF, isCygwin, isDarwin, isWindows
  } TargetType;
    
  /// This constructor initializes the data members to match that
  /// of the specified module.
  ///
  X86Subtarget(const Module &M, const std::string &FS);

  /// getStackAlignment - Returns the minimum alignment known to hold of the
  /// stack frame on entry to the function and which must be maintained by every
  /// function for this subtarget.
  unsigned getStackAlignment() const { return stackAlignment; }

  /// Returns true if the instruction selector should treat global values
  /// referencing external or weak symbols as indirect rather than direct
  /// references.
  bool getIndirectExternAndWeakGlobals() const {
    return indirectExternAndWeakGlobals;
  }

  /// ParseSubtargetFeatures - Parses features string setting specified 
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(const std::string &FS, const std::string &CPU);

  bool is64Bit() const { return Is64Bit; }

  bool hasMMX() const { return HasMMX; }
  bool hasSSE() const { return HasSSE; }
  bool hasSSE2() const { return HasSSE2; }
  bool hasSSE3() const { return HasSSE3; }
  bool has3DNow() const { return Has3DNow; }
  bool has3DNowA() const { return Has3DNowA; }
};
} // End llvm namespace

#endif
