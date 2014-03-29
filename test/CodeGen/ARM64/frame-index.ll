; RUN: llc -march=arm64 -mtriple=arm64-apple-ios < %s | FileCheck %s
; rdar://11935841

define void @t1() nounwind ssp {
entry:
; CHECK-LABEL: t1:
; CHECK-NOT: add x{{[0-9]+}}, sp
; CHECK: stp x28, x27, [sp, #-16]!
  %v = alloca [288 x i32], align 4
  unreachable
}
