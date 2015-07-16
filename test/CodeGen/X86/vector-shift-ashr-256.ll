; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx | FileCheck %s --check-prefix=ALL --check-prefix=AVX --check-prefix=AVX1
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx2 | FileCheck %s --check-prefix=ALL --check-prefix=AVX --check-prefix=AVX2

;
; Variable Shifts
;

define <4 x i64> @var_shift_v4i64(<4 x i64> %a, <4 x i64> %b) nounwind {
; AVX1-LABEL: var_shift_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vpextrq $1, %xmm2, %rax
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm3
; AVX1-NEXT:    vpextrq $1, %xmm3, %rcx
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm4
; AVX1-NEXT:    vmovq %xmm2, %rax
; AVX1-NEXT:    vmovq %xmm3, %rcx
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm2
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm4[0]
; AVX1-NEXT:    vpextrq $1, %xmm0, %rax
; AVX1-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm3
; AVX1-NEXT:    vmovq %xmm0, %rax
; AVX1-NEXT:    vmovq %xmm1, %rcx
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm0
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm3[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: var_shift_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX2-NEXT:    vpextrq $1, %xmm2, %rax
; AVX2-NEXT:    vextracti128 $1, %ymm1, %xmm3
; AVX2-NEXT:    vpextrq $1, %xmm3, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm4
; AVX2-NEXT:    vmovq %xmm2, %rax
; AVX2-NEXT:    vmovq %xmm3, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm2
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm4[0]
; AVX2-NEXT:    vpextrq $1, %xmm0, %rax
; AVX2-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm3
; AVX2-NEXT:    vmovq %xmm0, %rax
; AVX2-NEXT:    vmovq %xmm1, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm0
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm3[0]
; AVX2-NEXT:    vinserti128 $1, %xmm2, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <4 x i64> %a, %b
  ret <4 x i64> %shift
}

define <8 x i32> @var_shift_v8i32(<8 x i32> %a, <8 x i32> %b) nounwind {
; AVX1-LABEL: var_shift_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm3
; AVX1-NEXT:    vpsrldq {{.*#+}} xmm4 = xmm3[12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero
; AVX1-NEXT:    vpsrad %xmm4, %xmm2, %xmm4
; AVX1-NEXT:    vpsrlq $32, %xmm3, %xmm5
; AVX1-NEXT:    vpsrad %xmm5, %xmm2, %xmm5
; AVX1-NEXT:    vpblendw {{.*#+}} xmm4 = xmm5[0,1,2,3],xmm4[4,5,6,7]
; AVX1-NEXT:    vpxor %xmm5, %xmm5, %xmm5
; AVX1-NEXT:    vpunpckhdq {{.*#+}} xmm6 = xmm3[2],xmm5[2],xmm3[3],xmm5[3]
; AVX1-NEXT:    vpsrad %xmm6, %xmm2, %xmm6
; AVX1-NEXT:    vpmovzxdq {{.*#+}} xmm3 = xmm3[0],zero,xmm3[1],zero
; AVX1-NEXT:    vpsrad %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpblendw {{.*#+}} xmm2 = xmm2[0,1,2,3],xmm6[4,5,6,7]
; AVX1-NEXT:    vpblendw {{.*#+}} xmm2 = xmm2[0,1],xmm4[2,3],xmm2[4,5],xmm4[6,7]
; AVX1-NEXT:    vpsrldq {{.*#+}} xmm3 = xmm1[12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero
; AVX1-NEXT:    vpsrad %xmm3, %xmm0, %xmm3
; AVX1-NEXT:    vpsrlq $32, %xmm1, %xmm4
; AVX1-NEXT:    vpsrad %xmm4, %xmm0, %xmm4
; AVX1-NEXT:    vpblendw {{.*#+}} xmm3 = xmm4[0,1,2,3],xmm3[4,5,6,7]
; AVX1-NEXT:    vpunpckhdq {{.*#+}} xmm4 = xmm1[2],xmm5[2],xmm1[3],xmm5[3]
; AVX1-NEXT:    vpsrad %xmm4, %xmm0, %xmm4
; AVX1-NEXT:    vpmovzxdq {{.*#+}} xmm1 = xmm1[0],zero,xmm1[1],zero
; AVX1-NEXT:    vpsrad %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm4[4,5,6,7]
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1],xmm3[2,3],xmm0[4,5],xmm3[6,7]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: var_shift_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsravd %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <8 x i32> %a, %b
  ret <8 x i32> %shift
}

define <16 x i16> @var_shift_v16i16(<16 x i16> %a, <16 x i16> %b) nounwind {
; AVX1-LABEL: var_shift_v16i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm2
; AVX1-NEXT:    vpsllw $12, %xmm2, %xmm3
; AVX1-NEXT:    vpsllw $4, %xmm2, %xmm2
; AVX1-NEXT:    vpor %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm3
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm4
; AVX1-NEXT:    vpsraw $8, %xmm4, %xmm5
; AVX1-NEXT:    vpblendvb %xmm2, %xmm5, %xmm4, %xmm2
; AVX1-NEXT:    vpsraw $4, %xmm2, %xmm4
; AVX1-NEXT:    vpblendvb %xmm3, %xmm4, %xmm2, %xmm2
; AVX1-NEXT:    vpsraw $2, %xmm2, %xmm4
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm4, %xmm2, %xmm2
; AVX1-NEXT:    vpsraw $1, %xmm2, %xmm4
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm4, %xmm2, %xmm2
; AVX1-NEXT:    vpsllw $12, %xmm1, %xmm3
; AVX1-NEXT:    vpsllw $4, %xmm1, %xmm1
; AVX1-NEXT:    vpor %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm3
; AVX1-NEXT:    vpsraw $8, %xmm0, %xmm4
; AVX1-NEXT:    vpblendvb %xmm1, %xmm4, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm1
; AVX1-NEXT:    vpblendvb %xmm3, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm1
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm1
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: var_shift_v16i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpxor %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpunpckhwd {{.*#+}} ymm3 = ymm1[4],ymm2[4],ymm1[5],ymm2[5],ymm1[6],ymm2[6],ymm1[7],ymm2[7],ymm1[12],ymm2[12],ymm1[13],ymm2[13],ymm1[14],ymm2[14],ymm1[15],ymm2[15]
; AVX2-NEXT:    vpunpckhwd {{.*#+}} ymm4 = ymm0[4,4,5,5,6,6,7,7,12,12,13,13,14,14,15,15]
; AVX2-NEXT:    vpsravd %ymm3, %ymm4, %ymm3
; AVX2-NEXT:    vpsrld $16, %ymm3, %ymm3
; AVX2-NEXT:    vpunpcklwd {{.*#+}} ymm1 = ymm1[0],ymm2[0],ymm1[1],ymm2[1],ymm1[2],ymm2[2],ymm1[3],ymm2[3],ymm1[8],ymm2[8],ymm1[9],ymm2[9],ymm1[10],ymm2[10],ymm1[11],ymm2[11]
; AVX2-NEXT:    vpunpcklwd {{.*#+}} ymm0 = ymm0[0,0,1,1,2,2,3,3,8,8,9,9,10,10,11,11]
; AVX2-NEXT:    vpsravd %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpsrld $16, %ymm0, %ymm0
; AVX2-NEXT:    vpackusdw %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <16 x i16> %a, %b
  ret <16 x i16> %shift
}

define <32 x i8> @var_shift_v32i8(<32 x i8> %a, <32 x i8> %b) nounwind {
; AVX1-LABEL: var_shift_v32i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm2
; AVX1-NEXT:    vpsllw $5, %xmm2, %xmm2
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm3 = xmm0[8],xmm2[8],xmm0[9],xmm2[9],xmm0[10],xmm2[10],xmm0[11],xmm2[11],xmm0[12],xmm2[12],xmm0[13],xmm2[13],xmm0[14],xmm2[14],xmm0[15],xmm2[15]
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm4
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm5 = xmm0[8],xmm4[8],xmm0[9],xmm4[9],xmm0[10],xmm4[10],xmm0[11],xmm4[11],xmm0[12],xmm4[12],xmm0[13],xmm4[13],xmm0[14],xmm4[14],xmm0[15],xmm4[15]
; AVX1-NEXT:    vpsraw $4, %xmm5, %xmm6
; AVX1-NEXT:    vpblendvb %xmm3, %xmm6, %xmm5, %xmm5
; AVX1-NEXT:    vpsraw $2, %xmm5, %xmm6
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm6, %xmm5, %xmm5
; AVX1-NEXT:    vpsraw $1, %xmm5, %xmm6
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm6, %xmm5, %xmm3
; AVX1-NEXT:    vpsrlw $8, %xmm3, %xmm3
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm2 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm4 = xmm0[0],xmm4[0],xmm0[1],xmm4[1],xmm0[2],xmm4[2],xmm0[3],xmm4[3],xmm0[4],xmm4[4],xmm0[5],xmm4[5],xmm0[6],xmm4[6],xmm0[7],xmm4[7]
; AVX1-NEXT:    vpsraw $4, %xmm4, %xmm5
; AVX1-NEXT:    vpblendvb %xmm2, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $2, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendvb %xmm2, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $1, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendvb %xmm2, %xmm5, %xmm4, %xmm2
; AVX1-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX1-NEXT:    vpackuswb %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm3 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm4 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX1-NEXT:    vpsraw $4, %xmm4, %xmm5
; AVX1-NEXT:    vpblendvb %xmm3, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $2, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $1, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpblendvb %xmm3, %xmm5, %xmm4, %xmm3
; AVX1-NEXT:    vpsrlw $8, %xmm3, %xmm3
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm4
; AVX1-NEXT:    vpblendvb %xmm1, %xmm4, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm4
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpblendvb %xmm1, %xmm4, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm4
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpblendvb %xmm1, %xmm4, %xmm0, %xmm0
; AVX1-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX1-NEXT:    vpackuswb %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: var_shift_v32i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsllw $5, %ymm1, %ymm1
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm2 = ymm0[8],ymm1[8],ymm0[9],ymm1[9],ymm0[10],ymm1[10],ymm0[11],ymm1[11],ymm0[12],ymm1[12],ymm0[13],ymm1[13],ymm0[14],ymm1[14],ymm0[15],ymm1[15],ymm0[24],ymm1[24],ymm0[25],ymm1[25],ymm0[26],ymm1[26],ymm0[27],ymm1[27],ymm0[28],ymm1[28],ymm0[29],ymm1[29],ymm0[30],ymm1[30],ymm0[31],ymm1[31]
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm3 = ymm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31]
; AVX2-NEXT:    vpsraw $4, %ymm3, %ymm4
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm3
; AVX2-NEXT:    vpsraw $2, %ymm3, %ymm4
; AVX2-NEXT:    vpaddw %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm3
; AVX2-NEXT:    vpsraw $1, %ymm3, %ymm4
; AVX2-NEXT:    vpaddw %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm2
; AVX2-NEXT:    vpsrlw $8, %ymm2, %ymm2
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm1 = ymm0[0],ymm1[0],ymm0[1],ymm1[1],ymm0[2],ymm1[2],ymm0[3],ymm1[3],ymm0[4],ymm1[4],ymm0[5],ymm1[5],ymm0[6],ymm1[6],ymm0[7],ymm1[7],ymm0[16],ymm1[16],ymm0[17],ymm1[17],ymm0[18],ymm1[18],ymm0[19],ymm1[19],ymm0[20],ymm1[20],ymm0[21],ymm1[21],ymm0[22],ymm1[22],ymm0[23],ymm1[23]
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm0 = ymm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23]
; AVX2-NEXT:    vpsraw $4, %ymm0, %ymm3
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsraw $2, %ymm0, %ymm3
; AVX2-NEXT:    vpaddw %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsraw $1, %ymm0, %ymm3
; AVX2-NEXT:    vpaddw %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsrlw $8, %ymm0, %ymm0
; AVX2-NEXT:    vpackuswb %ymm2, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <32 x i8> %a, %b
  ret <32 x i8> %shift
}

;
; Uniform Variable Shifts
;

define <4 x i64> @splatvar_shift_v4i64(<4 x i64> %a, <4 x i64> %b) nounwind {
; AVX1-LABEL: splatvar_shift_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm1 = xmm1[0,0]
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vpextrq $1, %xmm2, %rdx
; AVX1-NEXT:    vpextrq $1, %xmm1, %rax
; AVX1-NEXT:    movb %al, %cl
; AVX1-NEXT:    sarq %cl, %rdx
; AVX1-NEXT:    vmovq %rdx, %xmm3
; AVX1-NEXT:    vmovq %xmm2, %rsi
; AVX1-NEXT:    vmovq %xmm1, %rdx
; AVX1-NEXT:    movb %dl, %cl
; AVX1-NEXT:    sarq %cl, %rsi
; AVX1-NEXT:    vmovq %rsi, %xmm1
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm3[0]
; AVX1-NEXT:    vpextrq $1, %xmm0, %rsi
; AVX1-NEXT:    movb %al, %cl
; AVX1-NEXT:    sarq %cl, %rsi
; AVX1-NEXT:    vmovq %rsi, %xmm2
; AVX1-NEXT:    vmovq %xmm0, %rax
; AVX1-NEXT:    movb %dl, %cl
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm0
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatvar_shift_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpbroadcastq %xmm1, %ymm1
; AVX2-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX2-NEXT:    vpextrq $1, %xmm2, %rax
; AVX2-NEXT:    vextracti128 $1, %ymm1, %xmm3
; AVX2-NEXT:    vpextrq $1, %xmm3, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm4
; AVX2-NEXT:    vmovq %xmm2, %rax
; AVX2-NEXT:    vmovq %xmm3, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm2
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm4[0]
; AVX2-NEXT:    vpextrq $1, %xmm0, %rax
; AVX2-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm3
; AVX2-NEXT:    vmovq %xmm0, %rax
; AVX2-NEXT:    vmovq %xmm1, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm0
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm3[0]
; AVX2-NEXT:    vinserti128 $1, %xmm2, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %splat = shufflevector <4 x i64> %b, <4 x i64> undef, <4 x i32> zeroinitializer
  %shift = ashr <4 x i64> %a, %splat
  ret <4 x i64> %shift
}

define <8 x i32> @splatvar_shift_v8i32(<8 x i32> %a, <8 x i32> %b) nounwind {
; AVX1-LABEL: splatvar_shift_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendw {{.*#+}} xmm1 = xmm1[0,1],xmm2[2,3,4,5,6,7]
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vpsrad %xmm1, %xmm2, %xmm2
; AVX1-NEXT:    vpsrad %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatvar_shift_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX2-NEXT:    vpblendw {{.*#+}} xmm1 = xmm1[0,1],xmm2[2,3,4,5,6,7]
; AVX2-NEXT:    vpsrad %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %splat = shufflevector <8 x i32> %b, <8 x i32> undef, <8 x i32> zeroinitializer
  %shift = ashr <8 x i32> %a, %splat
  ret <8 x i32> %shift
}

define <16 x i16> @splatvar_shift_v16i16(<16 x i16> %a, <16 x i16> %b) nounwind {
; AVX1-LABEL: splatvar_shift_v16i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vmovd %xmm1, %eax
; AVX1-NEXT:    movzwl %ax, %eax
; AVX1-NEXT:    vmovd %eax, %xmm1
; AVX1-NEXT:    vpsraw %xmm1, %xmm2, %xmm2
; AVX1-NEXT:    vpsraw %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatvar_shift_v16i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovd %xmm1, %eax
; AVX2-NEXT:    movzwl %ax, %eax
; AVX2-NEXT:    vmovd %eax, %xmm1
; AVX2-NEXT:    vpsraw %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %splat = shufflevector <16 x i16> %b, <16 x i16> undef, <16 x i32> zeroinitializer
  %shift = ashr <16 x i16> %a, %splat
  ret <16 x i16> %shift
}

define <32 x i8> @splatvar_shift_v32i8(<32 x i8> %a, <32 x i8> %b) nounwind {
; AVX1-LABEL: splatvar_shift_v32i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpshufb %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm2 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm3
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm4 = xmm0[8],xmm3[8],xmm0[9],xmm3[9],xmm0[10],xmm3[10],xmm0[11],xmm3[11],xmm0[12],xmm3[12],xmm0[13],xmm3[13],xmm0[14],xmm3[14],xmm0[15],xmm3[15]
; AVX1-NEXT:    vpsraw $4, %xmm4, %xmm5
; AVX1-NEXT:    vpblendvb %xmm2, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $2, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm6
; AVX1-NEXT:    vpblendvb %xmm6, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $1, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm6, %xmm6, %xmm9
; AVX1-NEXT:    vpblendvb %xmm9, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsrlw $8, %xmm4, %xmm8
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm3 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; AVX1-NEXT:    vpsraw $4, %xmm3, %xmm5
; AVX1-NEXT:    vpblendvb %xmm1, %xmm5, %xmm3, %xmm3
; AVX1-NEXT:    vpsraw $2, %xmm3, %xmm5
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm4
; AVX1-NEXT:    vpblendvb %xmm4, %xmm5, %xmm3, %xmm3
; AVX1-NEXT:    vpsraw $1, %xmm3, %xmm5
; AVX1-NEXT:    vpaddw %xmm4, %xmm4, %xmm7
; AVX1-NEXT:    vpblendvb %xmm7, %xmm5, %xmm3, %xmm3
; AVX1-NEXT:    vpsrlw $8, %xmm3, %xmm3
; AVX1-NEXT:    vpackuswb %xmm8, %xmm3, %xmm8
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm5 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX1-NEXT:    vpsraw $4, %xmm5, %xmm3
; AVX1-NEXT:    vpblendvb %xmm2, %xmm3, %xmm5, %xmm2
; AVX1-NEXT:    vpsraw $2, %xmm2, %xmm3
; AVX1-NEXT:    vpblendvb %xmm6, %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpsraw $1, %xmm2, %xmm3
; AVX1-NEXT:    vpblendvb %xmm9, %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm3
; AVX1-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm1
; AVX1-NEXT:    vpblendvb %xmm4, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm1
; AVX1-NEXT:    vpblendvb %xmm7, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX1-NEXT:    vpackuswb %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm8, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatvar_shift_v32i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpbroadcastb %xmm1, %ymm1
; AVX2-NEXT:    vpsllw $5, %ymm1, %ymm1
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm2 = ymm0[8],ymm1[8],ymm0[9],ymm1[9],ymm0[10],ymm1[10],ymm0[11],ymm1[11],ymm0[12],ymm1[12],ymm0[13],ymm1[13],ymm0[14],ymm1[14],ymm0[15],ymm1[15],ymm0[24],ymm1[24],ymm0[25],ymm1[25],ymm0[26],ymm1[26],ymm0[27],ymm1[27],ymm0[28],ymm1[28],ymm0[29],ymm1[29],ymm0[30],ymm1[30],ymm0[31],ymm1[31]
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm3 = ymm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31]
; AVX2-NEXT:    vpsraw $4, %ymm3, %ymm4
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm3
; AVX2-NEXT:    vpsraw $2, %ymm3, %ymm4
; AVX2-NEXT:    vpaddw %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm3
; AVX2-NEXT:    vpsraw $1, %ymm3, %ymm4
; AVX2-NEXT:    vpaddw %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm2
; AVX2-NEXT:    vpsrlw $8, %ymm2, %ymm2
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm1 = ymm0[0],ymm1[0],ymm0[1],ymm1[1],ymm0[2],ymm1[2],ymm0[3],ymm1[3],ymm0[4],ymm1[4],ymm0[5],ymm1[5],ymm0[6],ymm1[6],ymm0[7],ymm1[7],ymm0[16],ymm1[16],ymm0[17],ymm1[17],ymm0[18],ymm1[18],ymm0[19],ymm1[19],ymm0[20],ymm1[20],ymm0[21],ymm1[21],ymm0[22],ymm1[22],ymm0[23],ymm1[23]
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm0 = ymm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23]
; AVX2-NEXT:    vpsraw $4, %ymm0, %ymm3
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsraw $2, %ymm0, %ymm3
; AVX2-NEXT:    vpaddw %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsraw $1, %ymm0, %ymm3
; AVX2-NEXT:    vpaddw %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsrlw $8, %ymm0, %ymm0
; AVX2-NEXT:    vpackuswb %ymm2, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %splat = shufflevector <32 x i8> %b, <32 x i8> undef, <32 x i32> zeroinitializer
  %shift = ashr <32 x i8> %a, %splat
  ret <32 x i8> %shift
}

;
; Constant Shifts
;

define <4 x i64> @constant_shift_v4i64(<4 x i64> %a) nounwind {
; AVX1-LABEL: constant_shift_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vpextrq $1, %xmm1, %rax
; AVX1-NEXT:    sarq $62, %rax
; AVX1-NEXT:    vmovq %rax, %xmm2
; AVX1-NEXT:    vmovq %xmm1, %rax
; AVX1-NEXT:    sarq $31, %rax
; AVX1-NEXT:    vmovq %rax, %xmm1
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; AVX1-NEXT:    vpextrq $1, %xmm0, %rax
; AVX1-NEXT:    sarq $7, %rax
; AVX1-NEXT:    vmovq %rax, %xmm2
; AVX1-NEXT:    vmovq %xmm0, %rax
; AVX1-NEXT:    sarq %rax
; AVX1-NEXT:    vmovq %rax, %xmm0
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: constant_shift_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vextracti128 $1, %ymm0, %xmm1
; AVX2-NEXT:    vpextrq $1, %xmm1, %rax
; AVX2-NEXT:    sarq $62, %rax
; AVX2-NEXT:    vmovq %rax, %xmm2
; AVX2-NEXT:    vmovq %xmm1, %rax
; AVX2-NEXT:    sarq $31, %rax
; AVX2-NEXT:    vmovq %rax, %xmm1
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; AVX2-NEXT:    vpextrq $1, %xmm0, %rax
; AVX2-NEXT:    sarq $7, %rax
; AVX2-NEXT:    vmovq %rax, %xmm2
; AVX2-NEXT:    vmovq %xmm0, %rax
; AVX2-NEXT:    sarq %rax
; AVX2-NEXT:    vmovq %rax, %xmm0
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX2-NEXT:    vinserti128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <4 x i64> %a, <i64 1, i64 7, i64 31, i64 62>
  ret <4 x i64> %shift
}

define <8 x i32> @constant_shift_v8i32(<8 x i32> %a) nounwind {
; AVX1-LABEL: constant_shift_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsrad $7, %xmm0, %xmm1
; AVX1-NEXT:    vpsrad $5, %xmm0, %xmm2
; AVX1-NEXT:    vpblendw {{.*#+}} xmm1 = xmm2[0,1,2,3],xmm1[4,5,6,7]
; AVX1-NEXT:    vpsrad $6, %xmm0, %xmm2
; AVX1-NEXT:    vpsrad $4, %xmm0, %xmm3
; AVX1-NEXT:    vpblendw {{.*#+}} xmm2 = xmm3[0,1,2,3],xmm2[4,5,6,7]
; AVX1-NEXT:    vpblendw {{.*#+}} xmm1 = xmm2[0,1],xmm1[2,3],xmm2[4,5],xmm1[6,7]
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpsrad $7, %xmm0, %xmm2
; AVX1-NEXT:    vpsrad $9, %xmm0, %xmm3
; AVX1-NEXT:    vpblendw {{.*#+}} xmm2 = xmm3[0,1,2,3],xmm2[4,5,6,7]
; AVX1-NEXT:    vpsrad $8, %xmm0, %xmm0
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1],xmm2[2,3],xmm0[4,5],xmm2[6,7]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: constant_shift_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsravd {{.*}}(%rip), %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <8 x i32> %a, <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 8, i32 7>
  ret <8 x i32> %shift
}

define <16 x i16> @constant_shift_v16i16(<16 x i16> %a) nounwind {
; AVX1-LABEL: constant_shift_v16i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vpsraw $8, %xmm1, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [32896,37008,41120,45232,49344,53456,57568,61680]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpsraw $4, %xmm1, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [256,8480,16704,24928,33152,41376,49600,57824]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpsraw $2, %xmm1, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [512,16960,33408,49856,768,17216,33664,50112]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpsraw $1, %xmm1, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [1024,33920,1280,34176,1536,34432,1792,34688]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpsraw $8, %xmm0, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [0,4112,8224,12336,16448,20560,24672,28784]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [0,8224,16448,24672,32896,41120,49344,57568]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [0,16448,32896,49344,256,16704,33152,49600]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm2
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [0,32896,256,33152,512,33408,768,33664]
; AVX1-NEXT:    vpblendvb %xmm3, %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: constant_shift_v16i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpxor %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vmovdqa {{.*#+}} ymm2 = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
; AVX2-NEXT:    vpunpckhwd {{.*#+}} ymm3 = ymm2[4],ymm1[4],ymm2[5],ymm1[5],ymm2[6],ymm1[6],ymm2[7],ymm1[7],ymm2[12],ymm1[12],ymm2[13],ymm1[13],ymm2[14],ymm1[14],ymm2[15],ymm1[15]
; AVX2-NEXT:    vpunpckhwd {{.*#+}} ymm4 = ymm0[4,4,5,5,6,6,7,7,12,12,13,13,14,14,15,15]
; AVX2-NEXT:    vpsravd %ymm3, %ymm4, %ymm3
; AVX2-NEXT:    vpsrld $16, %ymm3, %ymm3
; AVX2-NEXT:    vpunpcklwd {{.*#+}} ymm1 = ymm2[0],ymm1[0],ymm2[1],ymm1[1],ymm2[2],ymm1[2],ymm2[3],ymm1[3],ymm2[8],ymm1[8],ymm2[9],ymm1[9],ymm2[10],ymm1[10],ymm2[11],ymm1[11]
; AVX2-NEXT:    vpunpcklwd {{.*#+}} ymm0 = ymm0[0,0,1,1,2,2,3,3,8,8,9,9,10,10,11,11]
; AVX2-NEXT:    vpsravd %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpsrld $16, %ymm0, %ymm0
; AVX2-NEXT:    vpackusdw %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <16 x i16> %a, <i16 0, i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8, i16 9, i16 10, i16 11, i16 12, i16 13, i16 14, i16 15>
  ret <16 x i16> %shift
}

define <32 x i8> @constant_shift_v32i8(<32 x i8> %a) nounwind {
; AVX1-LABEL: constant_shift_v32i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm1 = [0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0]
; AVX1-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm2 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm3
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm4 = xmm0[8],xmm3[8],xmm0[9],xmm3[9],xmm0[10],xmm3[10],xmm0[11],xmm3[11],xmm0[12],xmm3[12],xmm0[13],xmm3[13],xmm0[14],xmm3[14],xmm0[15],xmm3[15]
; AVX1-NEXT:    vpsraw $4, %xmm4, %xmm5
; AVX1-NEXT:    vpblendvb %xmm2, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $2, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm6
; AVX1-NEXT:    vpblendvb %xmm6, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsraw $1, %xmm4, %xmm5
; AVX1-NEXT:    vpaddw %xmm6, %xmm6, %xmm9
; AVX1-NEXT:    vpblendvb %xmm9, %xmm5, %xmm4, %xmm4
; AVX1-NEXT:    vpsrlw $8, %xmm4, %xmm8
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm3 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; AVX1-NEXT:    vpsraw $4, %xmm3, %xmm5
; AVX1-NEXT:    vpblendvb %xmm1, %xmm5, %xmm3, %xmm3
; AVX1-NEXT:    vpsraw $2, %xmm3, %xmm5
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm4
; AVX1-NEXT:    vpblendvb %xmm4, %xmm5, %xmm3, %xmm3
; AVX1-NEXT:    vpsraw $1, %xmm3, %xmm5
; AVX1-NEXT:    vpaddw %xmm4, %xmm4, %xmm7
; AVX1-NEXT:    vpblendvb %xmm7, %xmm5, %xmm3, %xmm3
; AVX1-NEXT:    vpsrlw $8, %xmm3, %xmm3
; AVX1-NEXT:    vpackuswb %xmm8, %xmm3, %xmm8
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm5 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX1-NEXT:    vpsraw $4, %xmm5, %xmm3
; AVX1-NEXT:    vpblendvb %xmm2, %xmm3, %xmm5, %xmm2
; AVX1-NEXT:    vpsraw $2, %xmm2, %xmm3
; AVX1-NEXT:    vpblendvb %xmm6, %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpsraw $1, %xmm2, %xmm3
; AVX1-NEXT:    vpblendvb %xmm9, %xmm3, %xmm2, %xmm2
; AVX1-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm3
; AVX1-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm1
; AVX1-NEXT:    vpblendvb %xmm4, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm1
; AVX1-NEXT:    vpblendvb %xmm7, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX1-NEXT:    vpackuswb %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm8, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: constant_shift_v32i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovdqa {{.*#+}} ymm1 = [0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0]
; AVX2-NEXT:    vpsllw $5, %ymm1, %ymm1
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm2 = ymm0[8],ymm1[8],ymm0[9],ymm1[9],ymm0[10],ymm1[10],ymm0[11],ymm1[11],ymm0[12],ymm1[12],ymm0[13],ymm1[13],ymm0[14],ymm1[14],ymm0[15],ymm1[15],ymm0[24],ymm1[24],ymm0[25],ymm1[25],ymm0[26],ymm1[26],ymm0[27],ymm1[27],ymm0[28],ymm1[28],ymm0[29],ymm1[29],ymm0[30],ymm1[30],ymm0[31],ymm1[31]
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm3 = ymm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31]
; AVX2-NEXT:    vpsraw $4, %ymm3, %ymm4
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm3
; AVX2-NEXT:    vpsraw $2, %ymm3, %ymm4
; AVX2-NEXT:    vpaddw %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm3
; AVX2-NEXT:    vpsraw $1, %ymm3, %ymm4
; AVX2-NEXT:    vpaddw %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpblendvb %ymm2, %ymm4, %ymm3, %ymm2
; AVX2-NEXT:    vpsrlw $8, %ymm2, %ymm2
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm1 = ymm0[0],ymm1[0],ymm0[1],ymm1[1],ymm0[2],ymm1[2],ymm0[3],ymm1[3],ymm0[4],ymm1[4],ymm0[5],ymm1[5],ymm0[6],ymm1[6],ymm0[7],ymm1[7],ymm0[16],ymm1[16],ymm0[17],ymm1[17],ymm0[18],ymm1[18],ymm0[19],ymm1[19],ymm0[20],ymm1[20],ymm0[21],ymm1[21],ymm0[22],ymm1[22],ymm0[23],ymm1[23]
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm0 = ymm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23]
; AVX2-NEXT:    vpsraw $4, %ymm0, %ymm3
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsraw $2, %ymm0, %ymm3
; AVX2-NEXT:    vpaddw %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsraw $1, %ymm0, %ymm3
; AVX2-NEXT:    vpaddw %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpblendvb %ymm1, %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpsrlw $8, %ymm0, %ymm0
; AVX2-NEXT:    vpackuswb %ymm2, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <32 x i8> %a, <i8 0, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 7, i8 6, i8 5, i8 4, i8 3, i8 2, i8 1, i8 0, i8 0, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 7, i8 6, i8 5, i8 4, i8 3, i8 2, i8 1, i8 0>
  ret <32 x i8> %shift
}

;
; Uniform Constant Shifts
;

define <4 x i64> @splatconstant_shift_v4i64(<4 x i64> %a) nounwind {
; AVX1-LABEL: splatconstant_shift_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vpsrad $7, %xmm1, %xmm2
; AVX1-NEXT:    vpsrlq $7, %xmm1, %xmm1
; AVX1-NEXT:    vpblendw {{.*#+}} xmm1 = xmm1[0,1],xmm2[2,3],xmm1[4,5],xmm2[6,7]
; AVX1-NEXT:    vpsrad $7, %xmm0, %xmm2
; AVX1-NEXT:    vpsrlq $7, %xmm0, %xmm0
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1],xmm2[2,3],xmm0[4,5],xmm2[6,7]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatconstant_shift_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsrad $7, %ymm0, %ymm1
; AVX2-NEXT:    vpsrlq $7, %ymm0, %ymm0
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2],ymm1[3],ymm0[4],ymm1[5],ymm0[6],ymm1[7]
; AVX2-NEXT:    retq
  %shift = ashr <4 x i64> %a, <i64 7, i64 7, i64 7, i64 7>
  ret <4 x i64> %shift
}

define <8 x i32> @splatconstant_shift_v8i32(<8 x i32> %a) nounwind {
; AVX1-LABEL: splatconstant_shift_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsrad $5, %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpsrad $5, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatconstant_shift_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsrad $5, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <8 x i32> %a, <i32 5, i32 5, i32 5, i32 5, i32 5, i32 5, i32 5, i32 5>
  ret <8 x i32> %shift
}

define <16 x i16> @splatconstant_shift_v16i16(<16 x i16> %a) nounwind {
; AVX1-LABEL: splatconstant_shift_v16i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsraw $3, %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpsraw $3, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatconstant_shift_v16i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsraw $3, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <16 x i16> %a, <i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3>
  ret <16 x i16> %shift
}

define <32 x i8> @splatconstant_shift_v32i8(<32 x i8> %a) nounwind {
; AVX1-LABEL: splatconstant_shift_v32i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vpsrlw $3, %xmm1, %xmm1
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm2 = [31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31]
; AVX1-NEXT:    vpand %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm3 = [16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16]
; AVX1-NEXT:    vpxor %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vpsubb %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vpsrlw $3, %xmm0, %xmm0
; AVX1-NEXT:    vpand %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vpxor %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsubb %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatconstant_shift_v32i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsrlw $3, %ymm0, %ymm0
; AVX2-NEXT:    vpand {{.*}}(%rip), %ymm0, %ymm0
; AVX2-NEXT:    vmovdqa {{.*#+}} ymm1 = [16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16]
; AVX2-NEXT:    vpxor %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpsubb %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
  %shift = ashr <32 x i8> %a, <i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3>
  ret <32 x i8> %shift
}
