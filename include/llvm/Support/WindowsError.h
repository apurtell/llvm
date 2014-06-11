//===-- WindowsError.h - Support for mapping windows errors to posix-------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_WINDOWS_ERROR_H
#define LLVM_SUPPORT_WINDOWS_ERROR_H

#ifdef _MSC_VER
#include <system_error>

namespace llvm {
std::error_code mapWindowsError(unsigned EV);
}
#endif

#endif
