; Tests for SSE2 and below, without SSE3+.
; RUN: llc < %s -mtriple=i386-apple-darwin10 -mcpu=pentium4 -O3 | FileCheck %s

define void @test1(<2 x double>* %r, <2 x double>* %A, double %B) nounwind  {
; CHECK-LABEL: test1:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movapd (%ecx), %xmm0
; CHECK-NEXT:    movlpd {{[0-9]+}}(%esp), %xmm0
; CHECK-NEXT:    movapd %xmm0, (%eax)
; CHECK-NEXT:    retl
	%tmp3 = load <2 x double>* %A, align 16
	%tmp7 = insertelement <2 x double> undef, double %B, i32 0
	%tmp9 = shufflevector <2 x double> %tmp3, <2 x double> %tmp7, <2 x i32> < i32 2, i32 1 >
	store <2 x double> %tmp9, <2 x double>* %r, align 16
	ret void
}

define void @test2(<2 x double>* %r, <2 x double>* %A, double %B) nounwind  {
; CHECK-LABEL: test2:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movapd (%ecx), %xmm0
; CHECK-NEXT:    movhpd {{[0-9]+}}(%esp), %xmm0
; CHECK-NEXT:    movapd %xmm0, (%eax)
; CHECK-NEXT:    retl
	%tmp3 = load <2 x double>* %A, align 16
	%tmp7 = insertelement <2 x double> undef, double %B, i32 0
	%tmp9 = shufflevector <2 x double> %tmp3, <2 x double> %tmp7, <2 x i32> < i32 0, i32 2 >
	store <2 x double> %tmp9, <2 x double>* %r, align 16
	ret void
}


define void @test3(<4 x float>* %res, <4 x float>* %A, <4 x float>* %B) nounwind {
; CHECK-LABEL: test3:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movaps (%edx), %xmm0
; CHECK-NEXT:    unpcklps {{.*#+}} xmm0 = xmm0[0],mem[0],xmm0[1],mem[1]
; CHECK-NEXT:    movaps %xmm0, (%eax)
; CHECK-NEXT:    retl
	%tmp = load <4 x float>* %B		; <<4 x float>> [#uses=2]
	%tmp3 = load <4 x float>* %A		; <<4 x float>> [#uses=2]
	%tmp.upgrd.1 = extractelement <4 x float> %tmp3, i32 0		; <float> [#uses=1]
	%tmp7 = extractelement <4 x float> %tmp, i32 0		; <float> [#uses=1]
	%tmp8 = extractelement <4 x float> %tmp3, i32 1		; <float> [#uses=1]
	%tmp9 = extractelement <4 x float> %tmp, i32 1		; <float> [#uses=1]
	%tmp10 = insertelement <4 x float> undef, float %tmp.upgrd.1, i32 0		; <<4 x float>> [#uses=1]
	%tmp11 = insertelement <4 x float> %tmp10, float %tmp7, i32 1		; <<4 x float>> [#uses=1]
	%tmp12 = insertelement <4 x float> %tmp11, float %tmp8, i32 2		; <<4 x float>> [#uses=1]
	%tmp13 = insertelement <4 x float> %tmp12, float %tmp9, i32 3		; <<4 x float>> [#uses=1]
	store <4 x float> %tmp13, <4 x float>* %res
	ret void
}

define void @test4(<4 x float> %X, <4 x float>* %res) nounwind {
; CHECK-LABEL: test4:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,0,3,0]
; CHECK-NEXT:    movdqa %xmm0, (%eax)
; CHECK-NEXT:    retl
	%tmp5 = shufflevector <4 x float> %X, <4 x float> undef, <4 x i32> < i32 2, i32 6, i32 3, i32 7 >		; <<4 x float>> [#uses=1]
	store <4 x float> %tmp5, <4 x float>* %res
	ret void
}

define <4 x i32> @test5(i8** %ptr) nounwind {
; CHECK-LABEL: test5:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl (%eax), %eax
; CHECK-NEXT:    movss (%eax), %xmm1
; CHECK-NEXT:    pxor %xmm0, %xmm0
; CHECK-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; CHECK-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; CHECK-NEXT:    retl
	%tmp = load i8** %ptr		; <i8*> [#uses=1]
	%tmp.upgrd.1 = bitcast i8* %tmp to float*		; <float*> [#uses=1]
	%tmp.upgrd.2 = load float* %tmp.upgrd.1		; <float> [#uses=1]
	%tmp.upgrd.3 = insertelement <4 x float> undef, float %tmp.upgrd.2, i32 0		; <<4 x float>> [#uses=1]
	%tmp9 = insertelement <4 x float> %tmp.upgrd.3, float 0.000000e+00, i32 1		; <<4 x float>> [#uses=1]
	%tmp10 = insertelement <4 x float> %tmp9, float 0.000000e+00, i32 2		; <<4 x float>> [#uses=1]
	%tmp11 = insertelement <4 x float> %tmp10, float 0.000000e+00, i32 3		; <<4 x float>> [#uses=1]
	%tmp21 = bitcast <4 x float> %tmp11 to <16 x i8>		; <<16 x i8>> [#uses=1]
	%tmp22 = shufflevector <16 x i8> %tmp21, <16 x i8> zeroinitializer, <16 x i32> < i32 0, i32 16, i32 1, i32 17, i32 2, i32 18, i32 3, i32 19, i32 4, i32 20, i32 5, i32 21, i32 6, i32 22, i32 7, i32 23 >		; <<16 x i8>> [#uses=1]
	%tmp31 = bitcast <16 x i8> %tmp22 to <8 x i16>		; <<8 x i16>> [#uses=1]
	%tmp.upgrd.4 = shufflevector <8 x i16> zeroinitializer, <8 x i16> %tmp31, <8 x i32> < i32 0, i32 8, i32 1, i32 9, i32 2, i32 10, i32 3, i32 11 >		; <<8 x i16>> [#uses=1]
	%tmp36 = bitcast <8 x i16> %tmp.upgrd.4 to <4 x i32>		; <<4 x i32>> [#uses=1]
	ret <4 x i32> %tmp36
}

define void @test6(<4 x float>* %res, <4 x float>* %A) nounwind {
; CHECK-LABEL: test6:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movaps (%ecx), %xmm0
; CHECK-NEXT:    movaps %xmm0, (%eax)
; CHECK-NEXT:    retl
  %tmp1 = load <4 x float>* %A            ; <<4 x float>> [#uses=1]
  %tmp2 = shufflevector <4 x float> %tmp1, <4 x float> undef, <4 x i32> < i32 0, i32 5, i32 6, i32 7 >          ; <<4 x float>> [#uses=1]
  store <4 x float> %tmp2, <4 x float>* %res
  ret void
}

define void @test7() nounwind {
; CHECK-LABEL: test7:
; CHECK:       ## BB#0:
; CHECK-NEXT:    xorps %xmm0, %xmm0
; CHECK-NEXT:    movaps %xmm0, 0
; CHECK-NEXT:    retl
  bitcast <4 x i32> zeroinitializer to <4 x float>                ; <<4 x float>>:1 [#uses=1]
  shufflevector <4 x float> %1, <4 x float> zeroinitializer, <4 x i32> zeroinitializer         ; <<4 x float>>:2 [#uses=1]
  store <4 x float> %2, <4 x float>* null
  ret void
}

@x = external global [4 x i32]

define <2 x i64> @test8() nounwind {
; CHECK-LABEL: test8:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl L_x$non_lazy_ptr, %eax
; CHECK-NEXT:    movups (%eax), %xmm0
; CHECK-NEXT:    retl
	%tmp = load i32* getelementptr ([4 x i32]* @x, i32 0, i32 0)		; <i32> [#uses=1]
	%tmp3 = load i32* getelementptr ([4 x i32]* @x, i32 0, i32 1)		; <i32> [#uses=1]
	%tmp5 = load i32* getelementptr ([4 x i32]* @x, i32 0, i32 2)		; <i32> [#uses=1]
	%tmp7 = load i32* getelementptr ([4 x i32]* @x, i32 0, i32 3)		; <i32> [#uses=1]
	%tmp.upgrd.1 = insertelement <4 x i32> undef, i32 %tmp, i32 0		; <<4 x i32>> [#uses=1]
	%tmp13 = insertelement <4 x i32> %tmp.upgrd.1, i32 %tmp3, i32 1		; <<4 x i32>> [#uses=1]
	%tmp14 = insertelement <4 x i32> %tmp13, i32 %tmp5, i32 2		; <<4 x i32>> [#uses=1]
	%tmp15 = insertelement <4 x i32> %tmp14, i32 %tmp7, i32 3		; <<4 x i32>> [#uses=1]
	%tmp16 = bitcast <4 x i32> %tmp15 to <2 x i64>		; <<2 x i64>> [#uses=1]
	ret <2 x i64> %tmp16
}

define <4 x float> @test9(i32 %dummy, float %a, float %b, float %c, float %d) nounwind {
; CHECK-LABEL: test9:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movups {{[0-9]+}}(%esp), %xmm0
; CHECK-NEXT:    retl
	%tmp = insertelement <4 x float> undef, float %a, i32 0		; <<4 x float>> [#uses=1]
	%tmp11 = insertelement <4 x float> %tmp, float %b, i32 1		; <<4 x float>> [#uses=1]
	%tmp12 = insertelement <4 x float> %tmp11, float %c, i32 2		; <<4 x float>> [#uses=1]
	%tmp13 = insertelement <4 x float> %tmp12, float %d, i32 3		; <<4 x float>> [#uses=1]
	ret <4 x float> %tmp13
}

define <4 x float> @test10(float %a, float %b, float %c, float %d) nounwind {
; CHECK-LABEL: test10:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movaps {{[0-9]+}}(%esp), %xmm0
; CHECK-NEXT:    retl
	%tmp = insertelement <4 x float> undef, float %a, i32 0		; <<4 x float>> [#uses=1]
	%tmp11 = insertelement <4 x float> %tmp, float %b, i32 1		; <<4 x float>> [#uses=1]
	%tmp12 = insertelement <4 x float> %tmp11, float %c, i32 2		; <<4 x float>> [#uses=1]
	%tmp13 = insertelement <4 x float> %tmp12, float %d, i32 3		; <<4 x float>> [#uses=1]
	ret <4 x float> %tmp13
}

define <2 x double> @test11(double %a, double %b) nounwind {
; CHECK-LABEL: test11:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movaps {{[0-9]+}}(%esp), %xmm0
; CHECK-NEXT:    retl
	%tmp = insertelement <2 x double> undef, double %a, i32 0		; <<2 x double>> [#uses=1]
	%tmp7 = insertelement <2 x double> %tmp, double %b, i32 1		; <<2 x double>> [#uses=1]
	ret <2 x double> %tmp7
}

define void @test12() nounwind {
; CHECK-LABEL: test12:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movaps 0, %xmm0
; CHECK-NEXT:    xorps %xmm1, %xmm1
; CHECK-NEXT:    movhlps {{.*#+}} xmm1 = xmm0[1],xmm1[1]
; CHECK-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],mem[2,3]
; CHECK-NEXT:    addps %xmm1, %xmm0
; CHECK-NEXT:    movaps %xmm0, 0
; CHECK-NEXT:    retl
  %tmp1 = load <4 x float>* null          ; <<4 x float>> [#uses=2]
  %tmp2 = shufflevector <4 x float> %tmp1, <4 x float> < float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00 >, <4 x i32> < i32 0, i32 1, i32 6, i32 7 >             ; <<4 x float>> [#uses=1]
  %tmp3 = shufflevector <4 x float> %tmp1, <4 x float> zeroinitializer, <4 x i32> < i32 2, i32 3, i32 6, i32 7 >                ; <<4 x float>> [#uses=1]
  %tmp4 = fadd <4 x float> %tmp2, %tmp3            ; <<4 x float>> [#uses=1]
  store <4 x float> %tmp4, <4 x float>* null
  ret void
}

define void @test13(<4 x float>* %res, <4 x float>* %A, <4 x float>* %B, <4 x float>* %C) nounwind {
; CHECK-LABEL: test13:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movaps (%edx), %xmm0
; CHECK-NEXT:    shufps {{.*#+}} xmm0 = xmm0[1,1],mem[0,1]
; CHECK-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,2,1,3]
; CHECK-NEXT:    movdqa %xmm0, (%eax)
; CHECK-NEXT:    retl
  %tmp3 = load <4 x float>* %B            ; <<4 x float>> [#uses=1]
  %tmp5 = load <4 x float>* %C            ; <<4 x float>> [#uses=1]
  %tmp11 = shufflevector <4 x float> %tmp3, <4 x float> %tmp5, <4 x i32> < i32 1, i32 4, i32 1, i32 5 >         ; <<4 x float>> [#uses=1]
  store <4 x float> %tmp11, <4 x float>* %res
  ret void
}

define <4 x float> @test14(<4 x float>* %x, <4 x float>* %y) nounwind {
; CHECK-LABEL: test14:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movaps (%ecx), %xmm1
; CHECK-NEXT:    movaps (%eax), %xmm2
; CHECK-NEXT:    movaps %xmm2, %xmm0
; CHECK-NEXT:    addps %xmm1, %xmm0
; CHECK-NEXT:    subps %xmm1, %xmm2
; CHECK-NEXT:    movlhps {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; CHECK-NEXT:    retl
  %tmp = load <4 x float>* %y             ; <<4 x float>> [#uses=2]
  %tmp5 = load <4 x float>* %x            ; <<4 x float>> [#uses=2]
  %tmp9 = fadd <4 x float> %tmp5, %tmp             ; <<4 x float>> [#uses=1]
  %tmp21 = fsub <4 x float> %tmp5, %tmp            ; <<4 x float>> [#uses=1]
  %tmp27 = shufflevector <4 x float> %tmp9, <4 x float> %tmp21, <4 x i32> < i32 0, i32 1, i32 4, i32 5 >                ; <<4 x float>> [#uses=1]
  ret <4 x float> %tmp27
}

define <4 x float> @test15(<4 x float>* %x, <4 x float>* %y) nounwind {
; CHECK-LABEL: test15:
; CHECK:       ## BB#0: ## %entry
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movaps (%ecx), %xmm0
; CHECK-NEXT:    movaps (%eax), %xmm1
; CHECK-NEXT:    movhlps {{.*#+}} xmm0 = xmm1[1],xmm0[1]
; CHECK-NEXT:    retl
entry:
  %tmp = load <4 x float>* %y             ; <<4 x float>> [#uses=1]
  %tmp3 = load <4 x float>* %x            ; <<4 x float>> [#uses=1]
  %tmp4 = shufflevector <4 x float> %tmp3, <4 x float> %tmp, <4 x i32> < i32 2, i32 3, i32 6, i32 7 >           ; <<4 x float>> [#uses=1]
  ret <4 x float> %tmp4
}

; PR8900

define  <2 x double> @test16(<4 x double> * nocapture %srcA, <2 x double>* nocapture %dst) {
; CHECK-LABEL: test16:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movapd 96(%eax), %xmm0
; CHECK-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],mem[0]
; CHECK-NEXT:    retl
  %i5 = getelementptr inbounds <4 x double>* %srcA, i32 3
  %i6 = load <4 x double>* %i5, align 32
  %i7 = shufflevector <4 x double> %i6, <4 x double> undef, <2 x i32> <i32 0, i32 2>
  ret <2 x double> %i7
}

; PR9009
define fastcc void @test17() nounwind {
; CHECK-LABEL: test17:
; CHECK:       ## BB#0: ## %entry
; CHECK-NEXT:    movaps {{.*#+}} xmm0 = <u,u,32768,32768>
; CHECK-NEXT:    movaps %xmm0, (%eax)
; CHECK-NEXT:    retl
entry:
  %0 = insertelement <4 x i32> undef, i32 undef, i32 1
  %1 = shufflevector <4 x i32> <i32 undef, i32 undef, i32 32768, i32 32768>, <4 x i32> %0, <4 x i32> <i32 4, i32 5, i32 2, i32 3>
  %2 = bitcast <4 x i32> %1 to <4 x float>
  store <4 x float> %2, <4 x float> * undef
  ret void
}

; PR9210
define <4 x float> @f(<4 x double>) nounwind {
; CHECK-LABEL: f:
; CHECK:       ## BB#0: ## %entry
; CHECK-NEXT:    cvtpd2ps %xmm1, %xmm1
; CHECK-NEXT:    cvtpd2ps %xmm0, %xmm0
; CHECK-NEXT:    movlhps {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; CHECK-NEXT:    retl
entry:
 %double2float.i = fptrunc <4 x double> %0 to <4 x float>
 ret <4 x float> %double2float.i
}

define <2 x i64> @test_insert_64_zext(<2 x i64> %i) {
; CHECK-LABEL: test_insert_64_zext:
; CHECK:       ## BB#0:
; CHECK-NEXT:    movq %xmm0, %xmm0
; CHECK-NEXT:    retl
  %1 = shufflevector <2 x i64> %i, <2 x i64> <i64 0, i64 undef>, <2 x i32> <i32 0, i32 2>
  ret <2 x i64> %1
}

define <4 x i32> @PR19721(<4 x i32> %i) {
; CHECK-LABEL: PR19721:
; CHECK:       ## BB#0:
; CHECK-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[1,0,0,0]
; CHECK-NEXT:    movd %xmm1, %eax
; CHECK-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[3,0,0,0]
; CHECK-NEXT:    movhlps {{.*#+}} xmm0 = xmm0[1,1]
; CHECK-NEXT:    movd %xmm0, %ecx
; CHECK-NEXT:    movd %xmm1, %edx
; CHECK-NEXT:    movd %edx, %xmm0
; CHECK-NEXT:    movd %ecx, %xmm1
; CHECK-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; CHECK-NEXT:    movd %eax, %xmm0
; CHECK-NEXT:    shufps {{.*#+}} xmm0 = xmm0[1,0],xmm1[0,1]
; CHECK-NEXT:    retl
  %bc = bitcast <4 x i32> %i to i128
  %insert = and i128 %bc, -4294967296
  %bc2 = bitcast i128 %insert to <4 x i32>
  ret <4 x i32> %bc2
}

define <4 x i32> @test_mul(<4 x i32> %x, <4 x i32> %y) {
; CHECK-LABEL: test_mul:
; CHECK:       ## BB#0:
; CHECK-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[1,0,3,0]
; CHECK-NEXT:    pmuludq %xmm1, %xmm0
; CHECK-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,0,3,0]
; CHECK-NEXT:    pmuludq %xmm2, %xmm1
; CHECK-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,2],xmm1[0,2]
; CHECK-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,2,1,3]
; CHECK-NEXT:    retl
  %m = mul <4 x i32> %x, %y
  ret <4 x i32> %m
}
