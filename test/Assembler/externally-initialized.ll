; RUN: llvm-as < %s | llvm-dis | llvm-as | llvm-dis | FileCheck %s
; RUN: verify-uselistorder %s -preserve-bc-use-list-order

; CHECK: @G = externally_initialized global i32 0

@G = externally_initialized global i32 0
