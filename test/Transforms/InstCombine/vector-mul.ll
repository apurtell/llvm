; RUN: opt < %s -instcombine -S | FileCheck %s

; Check that instcombine rewrites multiply by a vector
; of known constant power-of-2 elements with vector shift.

define <4 x i8> @Zero_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 0, i8 0, i8 0, i8 0>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @Zero_i8(
; CHECK: ret <4 x i8> zeroinitializer

define <4 x i8> @Identity_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 1, i8 1, i8 1, i8 1>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @Identity_i8(
; CHECK: ret <4 x i8> %InVec

define <4 x i8> @AddToSelf_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 2, i8 2, i8 2, i8 2>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @AddToSelf_i8(
; CHECK: shl <4 x i8> %InVec, <i8 1, i8 1, i8 1, i8 1>
; CHECK: ret

define <4 x i8> @SplatPow2Test1_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 4, i8 4, i8 4, i8 4>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @SplatPow2Test1_i8(
; CHECK: shl <4 x i8> %InVec, <i8 2, i8 2, i8 2, i8 2>
; CHECK: ret

define <4 x i8> @SplatPow2Test2_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 8, i8 8, i8 8, i8 8>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @SplatPow2Test2_i8(
; CHECK: shl <4 x i8> %InVec, <i8 3, i8 3, i8 3, i8 3>
; CHECK: ret

define <4 x i8> @MulTest1_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 1, i8 2, i8 4, i8 8>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @MulTest1_i8(
; CHECK: shl <4 x i8> %InVec, <i8 0, i8 1, i8 2, i8 3>
; CHECK: ret

define <4 x i8> @MulTest2_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 3, i8 3, i8 3, i8 3>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @MulTest2_i8(
; CHECK: mul <4 x i8> %InVec, <i8 3, i8 3, i8 3, i8 3>
; CHECK: ret

define <4 x i8> @MulTest3_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 4, i8 4, i8 2, i8 2>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @MulTest3_i8(
; CHECK: shl <4 x i8> %InVec, <i8 2, i8 2, i8 1, i8 1>
; CHECK: ret


define <4 x i8> @MulTest4_i8(<4 x i8> %InVec)  {
entry:
  %mul = mul <4 x i8> %InVec, <i8 4, i8 4, i8 0, i8 1>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @MulTest4_i8(
; CHECK: mul <4 x i8> %InVec, <i8 4, i8 4, i8 0, i8 1>
; CHECK: ret

define <4 x i16> @Zero_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 0, i16 0, i16 0, i16 0>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @Zero_i16(
; CHECK: ret <4 x i16> zeroinitializer

define <4 x i16> @Identity_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 1, i16 1, i16 1, i16 1>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @Identity_i16(
; CHECK: ret <4 x i16> %InVec

define <4 x i16> @AddToSelf_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 2, i16 2, i16 2, i16 2>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @AddToSelf_i16(
; CHECK: shl <4 x i16> %InVec, <i16 1, i16 1, i16 1, i16 1>
; CHECK: ret

define <4 x i16> @SplatPow2Test1_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 4, i16 4, i16 4, i16 4>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @SplatPow2Test1_i16(
; CHECK: shl <4 x i16> %InVec, <i16 2, i16 2, i16 2, i16 2>
; CHECK: ret

define <4 x i16> @SplatPow2Test2_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 8, i16 8, i16 8, i16 8>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @SplatPow2Test2_i16(
; CHECK: shl <4 x i16> %InVec, <i16 3, i16 3, i16 3, i16 3>
; CHECK: ret

define <4 x i16> @MulTest1_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 1, i16 2, i16 4, i16 8>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @MulTest1_i16(
; CHECK: shl <4 x i16> %InVec, <i16 0, i16 1, i16 2, i16 3>
; CHECK: ret

define <4 x i16> @MulTest2_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 3, i16 3, i16 3, i16 3>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @MulTest2_i16(
; CHECK: mul <4 x i16> %InVec, <i16 3, i16 3, i16 3, i16 3>
; CHECK: ret

define <4 x i16> @MulTest3_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 4, i16 4, i16 2, i16 2>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @MulTest3_i16(
; CHECK: shl <4 x i16> %InVec, <i16 2, i16 2, i16 1, i16 1>
; CHECK: ret

define <4 x i16> @MulTest4_i16(<4 x i16> %InVec)  {
entry:
  %mul = mul <4 x i16> %InVec, <i16 4, i16 4, i16 0, i16 2>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @MulTest4_i16(
; CHECK: mul <4 x i16> %InVec, <i16 4, i16 4, i16 0, i16 2>
; CHECK: ret

define <4 x i32> @Zero_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 0, i32 0, i32 0, i32 0>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @Zero_i32(
; CHECK: ret <4 x i32> zeroinitializer

define <4 x i32> @Identity_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 1, i32 1, i32 1, i32 1>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @Identity_i32(
; CHECK: ret <4 x i32> %InVec

define <4 x i32> @AddToSelf_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 2, i32 2, i32 2, i32 2>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @AddToSelf_i32(
; CHECK: shl <4 x i32> %InVec, <i32 1, i32 1, i32 1, i32 1>
; CHECK: ret


define <4 x i32> @SplatPow2Test1_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 4, i32 4, i32 4, i32 4>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @SplatPow2Test1_i32(
; CHECK: shl <4 x i32> %InVec, <i32 2, i32 2, i32 2, i32 2>
; CHECK: ret

define <4 x i32> @SplatPow2Test2_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 8, i32 8, i32 8, i32 8>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @SplatPow2Test2_i32(
; CHECK: shl <4 x i32> %InVec, <i32 3, i32 3, i32 3, i32 3>
; CHECK: ret

define <4 x i32> @MulTest1_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 1, i32 2, i32 4, i32 8>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @MulTest1_i32(
; CHECK: shl <4 x i32> %InVec, <i32 0, i32 1, i32 2, i32 3>
; CHECK: ret

define <4 x i32> @MulTest2_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 3, i32 3, i32 3, i32 3>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @MulTest2_i32(
; CHECK: mul <4 x i32> %InVec, <i32 3, i32 3, i32 3, i32 3>
; CHECK: ret

define <4 x i32> @MulTest3_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 4, i32 4, i32 2, i32 2>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @MulTest3_i32(
; CHECK: shl <4 x i32> %InVec, <i32 2, i32 2, i32 1, i32 1>
; CHECK: ret


define <4 x i32> @MulTest4_i32(<4 x i32> %InVec)  {
entry:
  %mul = mul <4 x i32> %InVec, <i32 4, i32 4, i32 0, i32 1>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @MulTest4_i32(
; CHECK: mul <4 x i32> %InVec, <i32 4, i32 4, i32 0, i32 1>
; CHECK: ret

define <4 x i64> @Zero_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 0, i64 0, i64 0, i64 0>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @Zero_i64(
; CHECK: ret <4 x i64> zeroinitializer

define <4 x i64> @Identity_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 1, i64 1, i64 1, i64 1>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @Identity_i64(
; CHECK: ret <4 x i64> %InVec

define <4 x i64> @AddToSelf_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 2, i64 2, i64 2, i64 2>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @AddToSelf_i64(
; CHECK: shl <4 x i64> %InVec, <i64 1, i64 1, i64 1, i64 1>
; CHECK: ret

define <4 x i64> @SplatPow2Test1_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 4, i64 4, i64 4, i64 4>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @SplatPow2Test1_i64(
; CHECK: shl <4 x i64> %InVec, <i64 2, i64 2, i64 2, i64 2>
; CHECK: ret

define <4 x i64> @SplatPow2Test2_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 8, i64 8, i64 8, i64 8>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @SplatPow2Test2_i64(
; CHECK: shl <4 x i64> %InVec, <i64 3, i64 3, i64 3, i64 3>
; CHECK: ret

define <4 x i64> @MulTest1_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 1, i64 2, i64 4, i64 8>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @MulTest1_i64(
; CHECK: shl <4 x i64> %InVec, <i64 0, i64 1, i64 2, i64 3>
; CHECK: ret

define <4 x i64> @MulTest2_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 3, i64 3, i64 3, i64 3>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @MulTest2_i64(
; CHECK: mul <4 x i64> %InVec, <i64 3, i64 3, i64 3, i64 3>
; CHECK: ret

define <4 x i64> @MulTest3_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 4, i64 4, i64 2, i64 2>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @MulTest3_i64(
; CHECK: shl <4 x i64> %InVec, <i64 2, i64 2, i64 1, i64 1>
; CHECK: ret

define <4 x i64> @MulTest4_i64(<4 x i64> %InVec)  {
entry:
  %mul = mul <4 x i64> %InVec, <i64 4, i64 4, i64 0, i64 1>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @MulTest4_i64(
; CHECK: mul <4 x i64> %InVec, <i64 4, i64 4, i64 0, i64 1>
; CHECK: ret

; Test also that the following rewriting rule works with vectors
; of integers as well:
;   ((X << C1)*C2) == (X * (C2 << C1))

define <4 x i8> @ShiftMulTest1(<4 x i8> %InVec) {
entry:
  %shl = shl <4 x i8> %InVec, <i8 2, i8 2, i8 2, i8 2>
  %mul = mul <4 x i8> %shl, <i8 3, i8 3, i8 3, i8 3>
  ret <4 x i8> %mul
}

; CHECK-LABEL: @ShiftMulTest1(
; CHECK: mul <4 x i8> %InVec, <i8 12, i8 12, i8 12, i8 12>
; CHECK: ret

define <4 x i16> @ShiftMulTest2(<4 x i16> %InVec) {
entry:
  %shl = shl <4 x i16> %InVec, <i16 2, i16 2, i16 2, i16 2>
  %mul = mul <4 x i16> %shl, <i16 3, i16 3, i16 3, i16 3>
  ret <4 x i16> %mul
}

; CHECK-LABEL: @ShiftMulTest2(
; CHECK: mul <4 x i16> %InVec, <i16 12, i16 12, i16 12, i16 12>
; CHECK: ret

define <4 x i32> @ShiftMulTest3(<4 x i32> %InVec) {
entry:
  %shl = shl <4 x i32> %InVec, <i32 2, i32 2, i32 2, i32 2>
  %mul = mul <4 x i32> %shl, <i32 3, i32 3, i32 3, i32 3>
  ret <4 x i32> %mul
}

; CHECK-LABEL: @ShiftMulTest3(
; CHECK: mul <4 x i32> %InVec, <i32 12, i32 12, i32 12, i32 12>
; CHECK: ret

define <4 x i64> @ShiftMulTest4(<4 x i64> %InVec) {
entry:
  %shl = shl <4 x i64> %InVec, <i64 2, i64 2, i64 2, i64 2>
  %mul = mul <4 x i64> %shl, <i64 3, i64 3, i64 3, i64 3>
  ret <4 x i64> %mul
}

; CHECK-LABEL: @ShiftMulTest4(
; CHECK: mul <4 x i64> %InVec, <i64 12, i64 12, i64 12, i64 12>
; CHECK: ret

