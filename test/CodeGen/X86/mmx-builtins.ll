; RUN: llc < %s -march=x86 -mattr=+mmx,+ssse3 | FileCheck %s
; RUN: llc < %s -march=x86 -mattr=+avx | FileCheck %s

declare x86_mmx @llvm.x86.ssse3.phadd.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test1(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: phaddw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %2 = bitcast <4 x i16> %1 to x86_mmx
  %3 = bitcast <4 x i16> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.phadd.w(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <4 x i16>
  %6 = bitcast <4 x i16> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.mmx.pcmpgt.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test88(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pcmpgtd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pcmpgt.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pcmpgt.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test87(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pcmpgtw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pcmpgt.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pcmpgt.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test86(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pcmpgtb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pcmpgt.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pcmpeq.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test85(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pcmpeqd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pcmpeq.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pcmpeq.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test84(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pcmpeqw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pcmpeq.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pcmpeq.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test83(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pcmpeqb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pcmpeq.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.punpckldq(x86_mmx, x86_mmx) nounwind readnone

define i64 @test82(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: punpckldq
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.punpckldq(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.punpcklwd(x86_mmx, x86_mmx) nounwind readnone

define i64 @test81(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: punpcklwd
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.punpcklwd(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.punpcklbw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test80(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: punpcklbw
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.punpcklbw(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.punpckhdq(x86_mmx, x86_mmx) nounwind readnone

define i64 @test79(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: punpckhdq
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.punpckhdq(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.punpckhwd(x86_mmx, x86_mmx) nounwind readnone

define i64 @test78(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: punpckhwd
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.punpckhwd(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.punpckhbw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test77(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: punpckhbw
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.punpckhbw(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.packuswb(x86_mmx, x86_mmx) nounwind readnone

define i64 @test76(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: packuswb
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.packuswb(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.packssdw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test75(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: packssdw
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.packssdw(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.packsswb(x86_mmx, x86_mmx) nounwind readnone

define i64 @test74(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: packsswb
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.packsswb(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psrai.d(x86_mmx, i32) nounwind readnone

define i64 @test73(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psrad
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.psrai.d(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to <2 x i32>
  %3 = bitcast <2 x i32> %2 to <1 x i64>
  %4 = extractelement <1 x i64> %3, i32 0
  ret i64 %4
}

declare x86_mmx @llvm.x86.mmx.psrai.w(x86_mmx, i32) nounwind readnone

define i64 @test72(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psraw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.psrai.w(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to <4 x i16>
  %3 = bitcast <4 x i16> %2 to <1 x i64>
  %4 = extractelement <1 x i64> %3, i32 0
  ret i64 %4
}

declare x86_mmx @llvm.x86.mmx.psrli.q(x86_mmx, i32) nounwind readnone

define i64 @test71(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psrlq
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var.i = bitcast i64 %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.psrli.q(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to i64
  ret i64 %2
}

declare x86_mmx @llvm.x86.mmx.psrli.d(x86_mmx, i32) nounwind readnone

define i64 @test70(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psrld
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.psrli.d(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to <2 x i32>
  %3 = bitcast <2 x i32> %2 to <1 x i64>
  %4 = extractelement <1 x i64> %3, i32 0
  ret i64 %4
}

declare x86_mmx @llvm.x86.mmx.psrli.w(x86_mmx, i32) nounwind readnone

define i64 @test69(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psrlw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.psrli.w(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to <4 x i16>
  %3 = bitcast <4 x i16> %2 to <1 x i64>
  %4 = extractelement <1 x i64> %3, i32 0
  ret i64 %4
}

declare x86_mmx @llvm.x86.mmx.pslli.q(x86_mmx, i32) nounwind readnone

define i64 @test68(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psllq
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var.i = bitcast i64 %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.pslli.q(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to i64
  ret i64 %2
}

declare x86_mmx @llvm.x86.mmx.pslli.d(x86_mmx, i32) nounwind readnone

define i64 @test67(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: pslld
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.pslli.d(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to <2 x i32>
  %3 = bitcast <2 x i32> %2 to <1 x i64>
  %4 = extractelement <1 x i64> %3, i32 0
  ret i64 %4
}

declare x86_mmx @llvm.x86.mmx.pslli.w(x86_mmx, i32) nounwind readnone

define i64 @test66(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: psllw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %0 to x86_mmx
  %1 = tail call x86_mmx @llvm.x86.mmx.pslli.w(x86_mmx %mmx_var.i, i32 3) nounwind
  %2 = bitcast x86_mmx %1 to <4 x i16>
  %3 = bitcast <4 x i16> %2 to <1 x i64>
  %4 = extractelement <1 x i64> %3, i32 0
  ret i64 %4
}

declare x86_mmx @llvm.x86.mmx.psra.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test65(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psrad
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psra.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psra.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test64(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psraw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psra.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psrl.q(x86_mmx, x86_mmx) nounwind readnone

define i64 @test63(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psrlq
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var.i = bitcast i64 %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psrl.q(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare x86_mmx @llvm.x86.mmx.psrl.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test62(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psrld
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psrl.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psrl.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test61(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psrlw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psrl.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psll.q(x86_mmx, x86_mmx) nounwind readnone

define i64 @test60(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psllq
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var.i = bitcast i64 %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psll.q(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare x86_mmx @llvm.x86.mmx.psll.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test59(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pslld
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psll.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psll.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test58(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psllw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1.i = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psll.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pxor(x86_mmx, x86_mmx) nounwind readnone

define i64 @test56(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pxor
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pxor(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.por(x86_mmx, x86_mmx) nounwind readnone

define i64 @test55(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: por
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.por(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pandn(x86_mmx, x86_mmx) nounwind readnone

define i64 @test54(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pandn
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pandn(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pand(x86_mmx, x86_mmx) nounwind readnone

define i64 @test53(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pand
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pand(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pmull.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test52(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmullw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmull.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

define i64 @test51(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmullw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmull.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pmulh.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test50(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmulhw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmulh.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pmadd.wd(x86_mmx, x86_mmx) nounwind readnone

define i64 @test49(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmaddwd
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmadd.wd(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psubus.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test48(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubusw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psubus.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psubus.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test47(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubusb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psubus.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psubs.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test46(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psubs.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psubs.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test45(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubsb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psubs.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

define i64 @test44(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubq
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var = bitcast i64 %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1 = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psub.q(x86_mmx %mmx_var, x86_mmx %mmx_var1)
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare x86_mmx @llvm.x86.mmx.psub.q(x86_mmx, x86_mmx) nounwind readnone

declare x86_mmx @llvm.x86.mmx.psub.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test43(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psub.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psub.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test42(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psub.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psub.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test41(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psubb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psub.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.paddus.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test40(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddusw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.paddus.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.paddus.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test39(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddusb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.paddus.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.padds.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test38(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.padds.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.padds.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test37(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddsb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.padds.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.padd.q(x86_mmx, x86_mmx) nounwind readnone

define i64 @test36(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddq
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var = bitcast i64 %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1 = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.padd.q(x86_mmx %mmx_var, x86_mmx %mmx_var1)
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare x86_mmx @llvm.x86.mmx.padd.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test35(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.padd.d(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.padd.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test34(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.padd.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.padd.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test33(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: paddb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.padd.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.psad.bw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test32(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psadbw
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.psad.bw(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare x86_mmx @llvm.x86.mmx.pmins.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test31(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pminsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmins.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pminu.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test30(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pminub
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pminu.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pmaxs.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test29(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmaxsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmaxs.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pmaxu.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test28(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmaxub
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmaxu.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pavg.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test27(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pavgw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pavg.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.mmx.pavg.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test26(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pavgb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pavg.b(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare void @llvm.x86.mmx.movnt.dq(x86_mmx*, x86_mmx) nounwind

define void @test25(<1 x i64>* %p, <1 x i64> %a) nounwind optsize ssp {
; CHECK: movntq
entry:
  %mmx_ptr_var.i = bitcast <1 x i64>* %p to x86_mmx*
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var.i = bitcast i64 %0 to x86_mmx
  tail call void @llvm.x86.mmx.movnt.dq(x86_mmx* %mmx_ptr_var.i, x86_mmx %mmx_var.i) nounwind
  ret void
}

declare i32 @llvm.x86.mmx.pmovmskb(x86_mmx) nounwind readnone

define i32 @test24(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: pmovmskb
entry:
  %0 = bitcast <1 x i64> %a to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %0 to x86_mmx
  %1 = tail call i32 @llvm.x86.mmx.pmovmskb(x86_mmx %mmx_var.i) nounwind
  ret i32 %1
}

declare void @llvm.x86.mmx.maskmovq(x86_mmx, x86_mmx, i8*) nounwind

define void @test23(<1 x i64> %d, <1 x i64> %n, i8* %p) nounwind optsize ssp {
; CHECK: maskmovq
entry:
  %0 = bitcast <1 x i64> %n to <8 x i8>
  %1 = bitcast <1 x i64> %d to <8 x i8>
  %mmx_var.i = bitcast <8 x i8> %1 to x86_mmx
  %mmx_var1.i = bitcast <8 x i8> %0 to x86_mmx
  tail call void @llvm.x86.mmx.maskmovq(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i, i8* %p) nounwind
  ret void
}

declare x86_mmx @llvm.x86.mmx.pmulhu.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test22(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmulhuw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %mmx_var.i = bitcast <4 x i16> %1 to x86_mmx
  %mmx_var1.i = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmulhu.w(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.sse.pshuf.w(x86_mmx, i8) nounwind readnone

define i64 @test21(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: pshufw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %1 = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.sse.pshuf.w(x86_mmx %1, i8 3) nounwind readnone
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

define i32 @test21_2(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: test21_2
; CHECK: pshufw
; CHECK: movd
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %1 = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.sse.pshuf.w(x86_mmx %1, i8 3) nounwind readnone
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <2 x i32>
  %5 = extractelement <2 x i32> %4, i32 0
  ret i32 %5
}

declare x86_mmx @llvm.x86.mmx.pmulu.dq(x86_mmx, x86_mmx) nounwind readnone

define i64 @test20(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmuludq
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %mmx_var.i = bitcast <2 x i32> %1 to x86_mmx
  %mmx_var1.i = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.pmulu.dq(x86_mmx %mmx_var.i, x86_mmx %mmx_var1.i) nounwind
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare <2 x double> @llvm.x86.sse.cvtpi2pd(x86_mmx) nounwind readnone

define <2 x double> @test19(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: cvtpi2pd
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %1 = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call <2 x double> @llvm.x86.sse.cvtpi2pd(x86_mmx %1) nounwind readnone
  ret <2 x double> %2
}

declare x86_mmx @llvm.x86.sse.cvttpd2pi(<2 x double>) nounwind readnone

define i64 @test18(<2 x double> %a) nounwind readnone optsize ssp {
; CHECK: cvttpd2pi
entry:
  %0 = tail call x86_mmx @llvm.x86.sse.cvttpd2pi(<2 x double> %a) nounwind readnone
  %1 = bitcast x86_mmx %0 to <2 x i32>
  %2 = bitcast <2 x i32> %1 to <1 x i64>
  %3 = extractelement <1 x i64> %2, i32 0
  ret i64 %3
}

declare x86_mmx @llvm.x86.sse.cvtpd2pi(<2 x double>) nounwind readnone

define i64 @test17(<2 x double> %a) nounwind readnone optsize ssp {
; CHECK: cvtpd2pi
entry:
  %0 = tail call x86_mmx @llvm.x86.sse.cvtpd2pi(<2 x double> %a) nounwind readnone
  %1 = bitcast x86_mmx %0 to <2 x i32>
  %2 = bitcast <2 x i32> %1 to <1 x i64>
  %3 = extractelement <1 x i64> %2, i32 0
  ret i64 %3
}

declare x86_mmx @llvm.x86.mmx.palignr.b(x86_mmx, x86_mmx, i8) nounwind readnone

define i64 @test16(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: palignr
entry:
  %0 = extractelement <1 x i64> %a, i32 0
  %mmx_var = bitcast i64 %0 to x86_mmx
  %1 = extractelement <1 x i64> %b, i32 0
  %mmx_var1 = bitcast i64 %1 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.mmx.palignr.b(x86_mmx %mmx_var, x86_mmx %mmx_var1, i8 16)
  %3 = bitcast x86_mmx %2 to i64
  ret i64 %3
}

declare x86_mmx @llvm.x86.ssse3.pabs.d(x86_mmx) nounwind readnone

define i64 @test15(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: pabsd
entry:
  %0 = bitcast <1 x i64> %a to <2 x i32>
  %1 = bitcast <2 x i32> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.ssse3.pabs.d(x86_mmx %1) nounwind readnone
  %3 = bitcast x86_mmx %2 to <2 x i32>
  %4 = bitcast <2 x i32> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.ssse3.pabs.w(x86_mmx) nounwind readnone

define i64 @test14(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: pabsw
entry:
  %0 = bitcast <1 x i64> %a to <4 x i16>
  %1 = bitcast <4 x i16> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.ssse3.pabs.w(x86_mmx %1) nounwind readnone
  %3 = bitcast x86_mmx %2 to <4 x i16>
  %4 = bitcast <4 x i16> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.ssse3.pabs.b(x86_mmx) nounwind readnone

define i64 @test13(<1 x i64> %a) nounwind readnone optsize ssp {
; CHECK: pabsb
entry:
  %0 = bitcast <1 x i64> %a to <8 x i8>
  %1 = bitcast <8 x i8> %0 to x86_mmx
  %2 = tail call x86_mmx @llvm.x86.ssse3.pabs.b(x86_mmx %1) nounwind readnone
  %3 = bitcast x86_mmx %2 to <8 x i8>
  %4 = bitcast <8 x i8> %3 to <1 x i64>
  %5 = extractelement <1 x i64> %4, i32 0
  ret i64 %5
}

declare x86_mmx @llvm.x86.ssse3.psign.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test12(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psignd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %2 = bitcast <2 x i32> %1 to x86_mmx
  %3 = bitcast <2 x i32> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.psign.d(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <2 x i32>
  %6 = bitcast <2 x i32> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.psign.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test11(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psignw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %2 = bitcast <4 x i16> %1 to x86_mmx
  %3 = bitcast <4 x i16> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.psign.w(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <4 x i16>
  %6 = bitcast <4 x i16> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.psign.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test10(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: psignb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %2 = bitcast <8 x i8> %1 to x86_mmx
  %3 = bitcast <8 x i8> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.psign.b(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <8 x i8>
  %6 = bitcast <8 x i8> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.pshuf.b(x86_mmx, x86_mmx) nounwind readnone

define i64 @test9(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pshufb
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %2 = bitcast <8 x i8> %1 to x86_mmx
  %3 = bitcast <8 x i8> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.pshuf.b(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <8 x i8>
  %6 = bitcast <8 x i8> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.pmul.hr.sw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test8(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmulhrsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %2 = bitcast <4 x i16> %1 to x86_mmx
  %3 = bitcast <4 x i16> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.pmul.hr.sw(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <4 x i16>
  %6 = bitcast <4 x i16> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.pmadd.ub.sw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test7(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: pmaddubsw
entry:
  %0 = bitcast <1 x i64> %b to <8 x i8>
  %1 = bitcast <1 x i64> %a to <8 x i8>
  %2 = bitcast <8 x i8> %1 to x86_mmx
  %3 = bitcast <8 x i8> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.pmadd.ub.sw(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <8 x i8>
  %6 = bitcast <8 x i8> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.phsub.sw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test6(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: phsubsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %2 = bitcast <4 x i16> %1 to x86_mmx
  %3 = bitcast <4 x i16> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.phsub.sw(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <4 x i16>
  %6 = bitcast <4 x i16> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.phsub.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test5(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: phsubd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %2 = bitcast <2 x i32> %1 to x86_mmx
  %3 = bitcast <2 x i32> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.phsub.d(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <2 x i32>
  %6 = bitcast <2 x i32> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.phsub.w(x86_mmx, x86_mmx) nounwind readnone

define i64 @test4(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: phsubw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %2 = bitcast <4 x i16> %1 to x86_mmx
  %3 = bitcast <4 x i16> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.phsub.w(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <4 x i16>
  %6 = bitcast <4 x i16> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.phadd.sw(x86_mmx, x86_mmx) nounwind readnone

define i64 @test3(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: phaddsw
entry:
  %0 = bitcast <1 x i64> %b to <4 x i16>
  %1 = bitcast <1 x i64> %a to <4 x i16>
  %2 = bitcast <4 x i16> %1 to x86_mmx
  %3 = bitcast <4 x i16> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.phadd.sw(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <4 x i16>
  %6 = bitcast <4 x i16> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

declare x86_mmx @llvm.x86.ssse3.phadd.d(x86_mmx, x86_mmx) nounwind readnone

define i64 @test2(<1 x i64> %a, <1 x i64> %b) nounwind readnone optsize ssp {
; CHECK: phaddd
entry:
  %0 = bitcast <1 x i64> %b to <2 x i32>
  %1 = bitcast <1 x i64> %a to <2 x i32>
  %2 = bitcast <2 x i32> %1 to x86_mmx
  %3 = bitcast <2 x i32> %0 to x86_mmx
  %4 = tail call x86_mmx @llvm.x86.ssse3.phadd.d(x86_mmx %2, x86_mmx %3) nounwind readnone
  %5 = bitcast x86_mmx %4 to <2 x i32>
  %6 = bitcast <2 x i32> %5 to <1 x i64>
  %7 = extractelement <1 x i64> %6, i32 0
  ret i64 %7
}

define <4 x float> @test89(<4 x float> %a, x86_mmx %b) nounwind {
; CHECK: cvtpi2ps
  %c = tail call <4 x float> @llvm.x86.sse.cvtpi2ps(<4 x float> %a, x86_mmx %b)
  ret <4 x float> %c
}

declare <4 x float> @llvm.x86.sse.cvtpi2ps(<4 x float>, x86_mmx) nounwind readnone
