; RUN: llc < %s -mtriple x86_64-unknown-unknown | FileCheck %s

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

; PR23246
; We're really only interested in doing something sane with the shuffle.

; CHECK-LABEL: test:
; CHECK:      movq2dq %mm0, %xmm0
; CHECK-NEXT: pshufd {{.*}} xmm0 = xmm0[0,1,0,1]
; CHECK-NEXT: retq
define <2 x i64> @test(x86_mmx %a) #0 {
entry:
  %b = bitcast x86_mmx %a to <1 x i64>
  %s = shufflevector <1 x i64> %b, <1 x i64> undef, <2 x i32> <i32 undef, i32 0>
  ret <2 x i64> %s
}

attributes #0 = { nounwind }
