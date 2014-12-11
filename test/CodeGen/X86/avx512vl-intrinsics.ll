; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=knl -mattr=+avx512vl --show-mc-encoding| FileCheck %s

; 256-bit

define i8 @test_pcmpeq_d_256(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: test_pcmpeq_d_256
; CHECK: vpcmpeqd %ymm1, %ymm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.d.256(<8 x i32> %a, <8 x i32> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpeq_d_256(<8 x i32> %a, <8 x i32> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpeq_d_256
; CHECK: vpcmpeqd %ymm1, %ymm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.d.256(<8 x i32> %a, <8 x i32> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpeq.d.256(<8 x i32>, <8 x i32>, i8)

define i8 @test_pcmpeq_q_256(<4 x i64> %a, <4 x i64> %b) {
; CHECK-LABEL: test_pcmpeq_q_256
; CHECK: vpcmpeqq %ymm1, %ymm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.q.256(<4 x i64> %a, <4 x i64> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpeq_q_256(<4 x i64> %a, <4 x i64> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpeq_q_256
; CHECK: vpcmpeqq %ymm1, %ymm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.q.256(<4 x i64> %a, <4 x i64> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpeq.q.256(<4 x i64>, <4 x i64>, i8)

define i8 @test_pcmpgt_d_256(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: test_pcmpgt_d_256
; CHECK: vpcmpgtd %ymm1, %ymm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.d.256(<8 x i32> %a, <8 x i32> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpgt_d_256(<8 x i32> %a, <8 x i32> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpgt_d_256
; CHECK: vpcmpgtd %ymm1, %ymm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.d.256(<8 x i32> %a, <8 x i32> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpgt.d.256(<8 x i32>, <8 x i32>, i8)

define i8 @test_pcmpgt_q_256(<4 x i64> %a, <4 x i64> %b) {
; CHECK-LABEL: test_pcmpgt_q_256
; CHECK: vpcmpgtq %ymm1, %ymm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.q.256(<4 x i64> %a, <4 x i64> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpgt_q_256(<4 x i64> %a, <4 x i64> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpgt_q_256
; CHECK: vpcmpgtq %ymm1, %ymm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.q.256(<4 x i64> %a, <4 x i64> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpgt.q.256(<4 x i64>, <4 x i64>, i8)

define <8 x i8> @test_cmp_d_256(<8 x i32> %a0, <8 x i32> %a1) {
; CHECK-LABEL: test_cmp_d_256
; CHECK: vpcmpeqd %ymm1, %ymm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltd %ymm1, %ymm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpled %ymm1, %ymm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordd %ymm1, %ymm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqd %ymm1, %ymm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltd %ymm1, %ymm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnled %ymm1, %ymm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordd %ymm1, %ymm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_cmp_d_256(<8 x i32> %a0, <8 x i32> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_cmp_d_256
; CHECK: vpcmpeqd %ymm1, %ymm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltd %ymm1, %ymm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpled %ymm1, %ymm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordd %ymm1, %ymm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqd %ymm1, %ymm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltd %ymm1, %ymm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnled %ymm1, %ymm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordd %ymm1, %ymm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.cmp.d.256(<8 x i32>, <8 x i32>, i32, i8) nounwind readnone

define <8 x i8> @test_ucmp_d_256(<8 x i32> %a0, <8 x i32> %a1) {
; CHECK-LABEL: test_ucmp_d_256
; CHECK: vpcmpequd %ymm1, %ymm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltud %ymm1, %ymm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleud %ymm1, %ymm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordud %ymm1, %ymm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequd %ymm1, %ymm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltud %ymm1, %ymm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleud %ymm1, %ymm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordud %ymm1, %ymm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_ucmp_d_256(<8 x i32> %a0, <8 x i32> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_ucmp_d_256
; CHECK: vpcmpequd %ymm1, %ymm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltud %ymm1, %ymm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleud %ymm1, %ymm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordud %ymm1, %ymm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequd %ymm1, %ymm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltud %ymm1, %ymm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleud %ymm1, %ymm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordud %ymm1, %ymm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32> %a0, <8 x i32> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.ucmp.d.256(<8 x i32>, <8 x i32>, i32, i8) nounwind readnone

define <8 x i8> @test_cmp_q_256(<4 x i64> %a0, <4 x i64> %a1) {
; CHECK-LABEL: test_cmp_q_256
; CHECK: vpcmpeqq %ymm1, %ymm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltq %ymm1, %ymm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleq %ymm1, %ymm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordq %ymm1, %ymm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqq %ymm1, %ymm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltq %ymm1, %ymm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleq %ymm1, %ymm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordq %ymm1, %ymm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_cmp_q_256(<4 x i64> %a0, <4 x i64> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_cmp_q_256
; CHECK: vpcmpeqq %ymm1, %ymm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltq %ymm1, %ymm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleq %ymm1, %ymm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordq %ymm1, %ymm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqq %ymm1, %ymm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltq %ymm1, %ymm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleq %ymm1, %ymm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordq %ymm1, %ymm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.cmp.q.256(<4 x i64>, <4 x i64>, i32, i8) nounwind readnone

define <8 x i8> @test_ucmp_q_256(<4 x i64> %a0, <4 x i64> %a1) {
; CHECK-LABEL: test_ucmp_q_256
; CHECK: vpcmpequq %ymm1, %ymm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltuq %ymm1, %ymm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleuq %ymm1, %ymm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunorduq %ymm1, %ymm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequq %ymm1, %ymm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltuq %ymm1, %ymm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleuq %ymm1, %ymm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmporduq %ymm1, %ymm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_ucmp_q_256(<4 x i64> %a0, <4 x i64> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_ucmp_q_256
; CHECK: vpcmpequq %ymm1, %ymm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltuq %ymm1, %ymm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleuq %ymm1, %ymm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunorduq %ymm1, %ymm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequq %ymm1, %ymm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltuq %ymm1, %ymm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleuq %ymm1, %ymm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmporduq %ymm1, %ymm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64> %a0, <4 x i64> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.ucmp.q.256(<4 x i64>, <4 x i64>, i32, i8) nounwind readnone

; 128-bit

define i8 @test_pcmpeq_d_128(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: test_pcmpeq_d_128
; CHECK: vpcmpeqd %xmm1, %xmm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.d.128(<4 x i32> %a, <4 x i32> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpeq_d_128(<4 x i32> %a, <4 x i32> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpeq_d_128
; CHECK: vpcmpeqd %xmm1, %xmm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.d.128(<4 x i32> %a, <4 x i32> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpeq.d.128(<4 x i32>, <4 x i32>, i8)

define i8 @test_pcmpeq_q_128(<2 x i64> %a, <2 x i64> %b) {
; CHECK-LABEL: test_pcmpeq_q_128
; CHECK: vpcmpeqq %xmm1, %xmm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.q.128(<2 x i64> %a, <2 x i64> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpeq_q_128(<2 x i64> %a, <2 x i64> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpeq_q_128
; CHECK: vpcmpeqq %xmm1, %xmm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpeq.q.128(<2 x i64> %a, <2 x i64> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpeq.q.128(<2 x i64>, <2 x i64>, i8)

define i8 @test_pcmpgt_d_128(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: test_pcmpgt_d_128
; CHECK: vpcmpgtd %xmm1, %xmm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.d.128(<4 x i32> %a, <4 x i32> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpgt_d_128(<4 x i32> %a, <4 x i32> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpgt_d_128
; CHECK: vpcmpgtd %xmm1, %xmm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.d.128(<4 x i32> %a, <4 x i32> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpgt.d.128(<4 x i32>, <4 x i32>, i8)

define i8 @test_pcmpgt_q_128(<2 x i64> %a, <2 x i64> %b) {
; CHECK-LABEL: test_pcmpgt_q_128
; CHECK: vpcmpgtq %xmm1, %xmm0, %k0 ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.q.128(<2 x i64> %a, <2 x i64> %b, i8 -1)
  ret i8 %res
}

define i8 @test_mask_pcmpgt_q_128(<2 x i64> %a, <2 x i64> %b, i8 %mask) {
; CHECK-LABEL: test_mask_pcmpgt_q_128
; CHECK: vpcmpgtq %xmm1, %xmm0, %k0 {%k1} ##
  %res = call i8 @llvm.x86.avx512.mask.pcmpgt.q.128(<2 x i64> %a, <2 x i64> %b, i8 %mask)
  ret i8 %res
}

declare i8 @llvm.x86.avx512.mask.pcmpgt.q.128(<2 x i64>, <2 x i64>, i8)

define <8 x i8> @test_cmp_d_128(<4 x i32> %a0, <4 x i32> %a1) {
; CHECK-LABEL: test_cmp_d_128
; CHECK: vpcmpeqd %xmm1, %xmm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltd %xmm1, %xmm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpled %xmm1, %xmm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordd %xmm1, %xmm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqd %xmm1, %xmm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltd %xmm1, %xmm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnled %xmm1, %xmm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordd %xmm1, %xmm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_cmp_d_128(<4 x i32> %a0, <4 x i32> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_cmp_d_128
; CHECK: vpcmpeqd %xmm1, %xmm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltd %xmm1, %xmm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpled %xmm1, %xmm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordd %xmm1, %xmm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqd %xmm1, %xmm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltd %xmm1, %xmm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnled %xmm1, %xmm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordd %xmm1, %xmm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.cmp.d.128(<4 x i32>, <4 x i32>, i32, i8) nounwind readnone

define <8 x i8> @test_ucmp_d_128(<4 x i32> %a0, <4 x i32> %a1) {
; CHECK-LABEL: test_ucmp_d_128
; CHECK: vpcmpequd %xmm1, %xmm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltud %xmm1, %xmm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleud %xmm1, %xmm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordud %xmm1, %xmm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequd %xmm1, %xmm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltud %xmm1, %xmm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleud %xmm1, %xmm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordud %xmm1, %xmm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_ucmp_d_128(<4 x i32> %a0, <4 x i32> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_ucmp_d_128
; CHECK: vpcmpequd %xmm1, %xmm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltud %xmm1, %xmm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleud %xmm1, %xmm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordud %xmm1, %xmm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequd %xmm1, %xmm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltud %xmm1, %xmm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleud %xmm1, %xmm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordud %xmm1, %xmm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32> %a0, <4 x i32> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.ucmp.d.128(<4 x i32>, <4 x i32>, i32, i8) nounwind readnone

define <8 x i8> @test_cmp_q_128(<2 x i64> %a0, <2 x i64> %a1) {
; CHECK-LABEL: test_cmp_q_128
; CHECK: vpcmpeqq %xmm1, %xmm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltq %xmm1, %xmm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleq %xmm1, %xmm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordq %xmm1, %xmm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqq %xmm1, %xmm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltq %xmm1, %xmm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleq %xmm1, %xmm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordq %xmm1, %xmm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_cmp_q_128(<2 x i64> %a0, <2 x i64> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_cmp_q_128
; CHECK: vpcmpeqq %xmm1, %xmm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltq %xmm1, %xmm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleq %xmm1, %xmm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunordq %xmm1, %xmm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpneqq %xmm1, %xmm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltq %xmm1, %xmm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleq %xmm1, %xmm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmpordq %xmm1, %xmm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.cmp.q.128(<2 x i64>, <2 x i64>, i32, i8) nounwind readnone

define <8 x i8> @test_ucmp_q_128(<2 x i64> %a0, <2 x i64> %a1) {
; CHECK-LABEL: test_ucmp_q_128
; CHECK: vpcmpequq %xmm1, %xmm0, %k0 ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 0, i8 -1)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltuq %xmm1, %xmm0, %k0 ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 1, i8 -1)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleuq %xmm1, %xmm0, %k0 ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 2, i8 -1)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunorduq %xmm1, %xmm0, %k0 ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 3, i8 -1)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequq %xmm1, %xmm0, %k0 ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 4, i8 -1)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltuq %xmm1, %xmm0, %k0 ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 5, i8 -1)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleuq %xmm1, %xmm0, %k0 ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 6, i8 -1)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmporduq %xmm1, %xmm0, %k0 ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 7, i8 -1)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

define <8 x i8> @test_mask_ucmp_q_128(<2 x i64> %a0, <2 x i64> %a1, i8 %mask) {
; CHECK-LABEL: test_mask_ucmp_q_128
; CHECK: vpcmpequq %xmm1, %xmm0, %k0 {%k1} ##
  %res0 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 0, i8 %mask)
  %vec0 = insertelement <8 x i8> undef, i8 %res0, i32 0
; CHECK: vpcmpltuq %xmm1, %xmm0, %k0 {%k1} ##
  %res1 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 1, i8 %mask)
  %vec1 = insertelement <8 x i8> %vec0, i8 %res1, i32 1
; CHECK: vpcmpleuq %xmm1, %xmm0, %k0 {%k1} ##
  %res2 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 2, i8 %mask)
  %vec2 = insertelement <8 x i8> %vec1, i8 %res2, i32 2
; CHECK: vpcmpunorduq %xmm1, %xmm0, %k0 {%k1} ##
  %res3 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 3, i8 %mask)
  %vec3 = insertelement <8 x i8> %vec2, i8 %res3, i32 3
; CHECK: vpcmpnequq %xmm1, %xmm0, %k0 {%k1} ##
  %res4 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 4, i8 %mask)
  %vec4 = insertelement <8 x i8> %vec3, i8 %res4, i32 4
; CHECK: vpcmpnltuq %xmm1, %xmm0, %k0 {%k1} ##
  %res5 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 5, i8 %mask)
  %vec5 = insertelement <8 x i8> %vec4, i8 %res5, i32 5
; CHECK: vpcmpnleuq %xmm1, %xmm0, %k0 {%k1} ##
  %res6 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 6, i8 %mask)
  %vec6 = insertelement <8 x i8> %vec5, i8 %res6, i32 6
; CHECK: vpcmporduq %xmm1, %xmm0, %k0 {%k1} ##
  %res7 = call i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64> %a0, <2 x i64> %a1, i32 7, i8 %mask)
  %vec7 = insertelement <8 x i8> %vec6, i8 %res7, i32 7
  ret <8 x i8> %vec7
}

declare i8 @llvm.x86.avx512.mask.ucmp.q.128(<2 x i64>, <2 x i64>, i32, i8) nounwind readnone

; CHECK-LABEL: compr1
; CHECK: vcompresspd %zmm0
define void @compr1(i8* %addr, <8 x double> %data, i8 %mask) {
  call void @llvm.x86.avx512.mask.compress.store.pd.512(i8* %addr, <8 x double> %data, i8 %mask)
  ret void
}

declare void @llvm.x86.avx512.mask.compress.store.pd.512(i8* %addr, <8 x double> %data, i8 %mask)

; CHECK-LABEL: compr2
; CHECK: vcompresspd %ymm0
define void @compr2(i8* %addr, <4 x double> %data, i8 %mask) {
  call void @llvm.x86.avx512.mask.compress.store.pd.256(i8* %addr, <4 x double> %data, i8 %mask)
  ret void
}

declare void @llvm.x86.avx512.mask.compress.store.pd.256(i8* %addr, <4 x double> %data, i8 %mask)

; CHECK-LABEL: compr3
; CHECK: vcompressps %xmm0
define void @compr3(i8* %addr, <4 x float> %data, i8 %mask) {
  call void @llvm.x86.avx512.mask.compress.store.ps.128(i8* %addr, <4 x float> %data, i8 %mask)
  ret void
}

declare void @llvm.x86.avx512.mask.compress.store.ps.128(i8* %addr, <4 x float> %data, i8 %mask)

; CHECK-LABEL: compr4
; CHECK: vcompresspd %zmm0, %zmm0 {%k1} {z} ## encoding: [0x62,0xf2,0xfd,0xc9,0x8a,0xc0]
define <8 x double> @compr4(i8* %addr, <8 x double> %data, i8 %mask) {
  %res = call <8 x double> @llvm.x86.avx512.mask.compress.pd.512(<8 x double> %data, <8 x double> zeroinitializer, i8 %mask)
  ret <8 x double> %res
}

declare <8 x double> @llvm.x86.avx512.mask.compress.pd.512(<8 x double> %data, <8 x double> %src0, i8 %mask)

; CHECK-LABEL: compr5
; CHECK: vcompresspd %ymm0, %ymm1 {%k1}  ## encoding: [0x62,0xf2,0xfd,0x29,0x8a,0xc1]
define <4 x double> @compr5(<4 x double> %data, <4 x double> %src0, i8 %mask) {
  %res = call <4 x double> @llvm.x86.avx512.mask.compress.pd.256( <4 x double> %data, <4 x double> %src0, i8 %mask)
  ret <4 x double> %res
}

declare <4 x double> @llvm.x86.avx512.mask.compress.pd.256(<4 x double> %data, <4 x double> %src0, i8 %mask)

; CHECK-LABEL: compr6
; CHECK: vcompressps %xmm0
define <4 x float> @compr6(<4 x float> %data, i8 %mask) {
  %res = call <4 x float> @llvm.x86.avx512.mask.compress.ps.128(<4 x float> %data, <4 x float>zeroinitializer, i8 %mask)
  ret <4 x float> %res
}

declare <4 x float> @llvm.x86.avx512.mask.compress.ps.128(<4 x float> %data, <4 x float> %src0, i8 %mask)

; CHECK-LABEL: compr7
; CHECK-NOT: vcompress
; CHECK: vmovapd
define void @compr7(i8* %addr, <8 x double> %data) {
  call void @llvm.x86.avx512.mask.compress.store.pd.512(i8* %addr, <8 x double> %data, i8 -1)
  ret void
}

; CHECK-LABEL: compr8
; CHECK-NOT: vcompressps %xmm0
define <4 x float> @compr8(<4 x float> %data) {
  %res = call <4 x float> @llvm.x86.avx512.mask.compress.ps.128(<4 x float> %data, <4 x float>zeroinitializer, i8 -1)
  ret <4 x float> %res
}

; CHECK-LABEL: compr9
; CHECK: vpcompressq %zmm0, (%rdi) {%k1}  ## encoding: [0x62,0xf2,0xfd,0x49,0x8b,0x07]
define void @compr9(i8* %addr, <8 x i64> %data, i8 %mask) {
  call void @llvm.x86.avx512.mask.compress.store.q.512(i8* %addr, <8 x i64> %data, i8 %mask)
  ret void
}

declare void @llvm.x86.avx512.mask.compress.store.q.512(i8* %addr, <8 x i64> %data, i8 %mask)

; CHECK-LABEL: compr10
; CHECK: vpcompressd %xmm0, %xmm0 {%k1} {z} ## encoding: [0x62,0xf2,0x7d,0x89,0x8b,0xc0]
define <4 x i32> @compr10(<4 x i32> %data, i8 %mask) {
  %res = call <4 x i32> @llvm.x86.avx512.mask.compress.d.128(<4 x i32> %data, <4 x i32>zeroinitializer, i8 %mask)
  ret <4 x i32> %res
}

declare <4 x i32> @llvm.x86.avx512.mask.compress.d.128(<4 x i32> %data, <4 x i32> %src0, i8 %mask)
