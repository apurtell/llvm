; RUN: llvm-dis < %s.bc| FileCheck %s
; RUN: verify-uselistorder < %s.bc -preserve-bc-use-list-order

; BinaryFloatOperation.3.2.ll.bc was generated by passing this file to llvm-as-3.2.
; The test checks that LLVM does not misread binary float instructions from
; older bitcode files.

define void @fadd(float %x1, double %x2 ,half %x3, fp128 %x4, x86_fp80 %x5, ppc_fp128 %x6){
entry:
; CHECK: %res1 = fadd float %x1, %x1
  %res1 = fadd float %x1, %x1

; CHECK-NEXT: %res2 = fadd double %x2, %x2
  %res2 = fadd double %x2, %x2

; CHECK-NEXT: %res3 = fadd half %x3, %x3
  %res3 = fadd half %x3, %x3

; CHECK-NEXT: %res4 = fadd fp128 %x4, %x4
  %res4 = fadd fp128 %x4, %x4

; CHECK-NEXT: %res5 = fadd x86_fp80 %x5, %x5
  %res5 = fadd x86_fp80 %x5, %x5

; CHECK-NEXT: %res6 = fadd ppc_fp128 %x6, %x6
  %res6 = fadd ppc_fp128 %x6, %x6

  ret void
}

define void @faddFloatVec(<2 x float> %x1, <3 x float> %x2 ,<4 x float> %x3, <8 x float> %x4, <16 x float> %x5){
entry:
; CHECK: %res1 = fadd <2 x float> %x1, %x1
  %res1 = fadd <2 x float> %x1, %x1

; CHECK-NEXT: %res2 = fadd <3 x float> %x2, %x2
  %res2 = fadd <3 x float> %x2, %x2

; CHECK-NEXT: %res3 = fadd <4 x float> %x3, %x3
  %res3 = fadd <4 x float> %x3, %x3

; CHECK-NEXT: %res4 = fadd <8 x float> %x4, %x4
  %res4 = fadd <8 x float> %x4, %x4

; CHECK-NEXT: %res5 = fadd <16 x float> %x5, %x5
  %res5 = fadd <16 x float> %x5, %x5

  ret void
}

define void @faddDoubleVec(<2 x double> %x1, <3 x double> %x2 ,<4 x double> %x3, <8 x double> %x4, <16 x double> %x5){
entry:
; CHECK: %res1 = fadd <2 x double> %x1, %x1
  %res1 = fadd <2 x double> %x1, %x1

; CHECK-NEXT: %res2 = fadd <3 x double> %x2, %x2
  %res2 = fadd <3 x double> %x2, %x2

; CHECK-NEXT: %res3 = fadd <4 x double> %x3, %x3
  %res3 = fadd <4 x double> %x3, %x3

; CHECK-NEXT: %res4 = fadd <8 x double> %x4, %x4
  %res4 = fadd <8 x double> %x4, %x4

; CHECK-NEXT: %res5 = fadd <16 x double> %x5, %x5
  %res5 = fadd <16 x double> %x5, %x5

  ret void
}

define void @faddHalfVec(<2 x half> %x1, <3 x half> %x2 ,<4 x half> %x3, <8 x half> %x4, <16 x half> %x5){
entry:
; CHECK: %res1 = fadd <2 x half> %x1, %x1
  %res1 = fadd <2 x half> %x1, %x1

; CHECK-NEXT: %res2 = fadd <3 x half> %x2, %x2
  %res2 = fadd <3 x half> %x2, %x2

; CHECK-NEXT: %res3 = fadd <4 x half> %x3, %x3
  %res3 = fadd <4 x half> %x3, %x3

; CHECK-NEXT: %res4 = fadd <8 x half> %x4, %x4
  %res4 = fadd <8 x half> %x4, %x4

; CHECK-NEXT: %res5 = fadd <16 x half> %x5, %x5
  %res5 = fadd <16 x half> %x5, %x5

  ret void
}

define void @fsub(float %x1){
entry:
; CHECK: %res1 = fsub float %x1, %x1
  %res1 = fsub float %x1, %x1

  ret void
}

define void @fmul(float %x1){
entry:
; CHECK: %res1 = fmul float %x1, %x1
  %res1 = fmul float %x1, %x1

  ret void
}

define void @fdiv(float %x1){
entry:
; CHECK: %res1 = fdiv float %x1, %x1
  %res1 = fdiv float %x1, %x1

  ret void
}

define void @frem(float %x1){
entry:
; CHECK: %res1 = frem float %x1, %x1
  %res1 = frem float %x1, %x1

  ret void
}
