; RUN: llc -mtriple=x86_64-unknown-unknown -mattr=+sse4.1 < %s | FileCheck %s

; Verify that the DAGCombiner doesn't wrongly fold a build_vector into a
; blend with a zero vector if the build_vector contains negative zero.
;
; TODO: the codegen for function 'test_negative_zero_1' is sub-optimal.
; Ideally, we should generate a single shuffle blend operation.

define <4 x float> @test_negative_zero_1(<4 x float> %A) {
; CHECK-LABEL: test_negative_zero_1:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    movapd %xmm0, %xmm1
; CHECK-NEXT:    shufpd {{.*#+}} xmm1 = xmm1[1,0]
; CHECK-NEXT:    xorps %xmm2, %xmm2
; CHECK-NEXT:    blendps {{.*#+}} xmm2 = xmm1[0],xmm2[1,2,3]
; CHECK-NEXT:    movss {{.*#+}} xmm1 = mem[0],zero,zero,zero
; CHECK-NEXT:    unpcklps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; CHECK-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; CHECK-NEXT:    retq
entry:
  %0 = extractelement <4 x float> %A, i32 0
  %1 = insertelement <4 x float> undef, float %0, i32 0
  %2 = insertelement <4 x float> %1, float -0.0, i32 1
  %3 = extractelement <4 x float> %A, i32 2
  %4 = insertelement <4 x float> %2, float %3, i32 2
  %5 = insertelement <4 x float> %4, float 0.0, i32 3
  ret <4 x float> %5
}

define <2 x double> @test_negative_zero_2(<2 x double> %A) {
; CHECK-LABEL: test_negative_zero_2:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    movhpd {{.*}}(%rip), %xmm0
; CHECK-NEXT:    retq
entry:
  %0 = extractelement <2 x double> %A, i32 0
  %1 = insertelement <2 x double> undef, double %0, i32 0
  %2 = insertelement <2 x double> %1, double -0.0, i32 1
  ret <2 x double> %2
}
