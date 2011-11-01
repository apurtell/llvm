; RUN: llc < %s -march=xcore | FileCheck %s
declare double @llvm.log10.f64(double)

define double @test(double %F) {
; CHECK: test:
; CHECK: bl log10
        %result = call double @llvm.log10.f64(double %F)
	ret double %result
}

declare float @llvm.log10.f32(float)

define float @testf(float %F) {
; CHECK: testf:
; CHECK: bl log10f
        %result = call float @llvm.log10.f32(float %F)
	ret float %result
}
