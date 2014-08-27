; RUN: llc < %s -mtriple=aarch64-none-eabi | FileCheck %s

define <4 x i16> @v4f16_to_v4i16(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_v4i16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x half> %a to <4 x i16>
  ret <4 x i16> %1
}

define <2 x i32> @v4f16_to_v2i32(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_v2i32:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x half> %a to <2 x i32>
  ret <2 x i32> %1
}

define <1 x i64> @v4f16_to_v1i64(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_v1i64:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x half> %a to <1 x i64>
  ret <1 x i64> %1
}

define i64 @v4f16_to_i64(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_i64:
; CHECK: fmov x0, d1
entry:
  %1 = bitcast <4 x half> %a to i64
  ret i64 %1
}

define <2 x float> @v4f16_to_v2float(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_v2float:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x half> %a to <2 x float>
  ret <2 x float> %1
}

define <1 x double> @v4f16_to_v1double(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_v1double:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x half> %a to <1 x double>
  ret <1 x double> %1
}

define double @v4f16_to_double(float, <4 x half> %a) #0 {
; CHECK-LABEL: v4f16_to_double:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x half> %a to double
  ret double %1
}


define <4 x half> @v4i16_to_v4f16(float, <4 x i16> %a) #0 {
; CHECK-LABEL: v4i16_to_v4f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x i16> %a to <4 x half>
  ret <4 x half> %1
}

define <4 x half> @v2i32_to_v4f16(float, <2 x i32> %a) #0 {
; CHECK-LABEL: v2i32_to_v4f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <2 x i32> %a to <4 x half>
  ret <4 x half> %1
}

define <4 x half> @v1i64_to_v4f16(float, <1 x i64> %a) #0 {
; CHECK-LABEL: v1i64_to_v4f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <1 x i64> %a to <4 x half>
  ret <4 x half> %1
}

define <4 x half> @i64_to_v4f16(float, i64 %a) #0 {
; CHECK-LABEL: i64_to_v4f16:
; CHECK: fmov d0, x0
entry:
  %1 = bitcast i64 %a to <4 x half>
  ret <4 x half> %1
}

define <4 x half> @v2float_to_v4f16(float, <2 x float> %a) #0 {
; CHECK-LABEL: v2float_to_v4f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <2 x float> %a to <4 x half>
  ret <4 x half> %1
}

define <4 x half> @v1double_to_v4f16(float, <1 x double> %a) #0 {
; CHECK-LABEL: v1double_to_v4f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <1 x double> %a to <4 x half>
  ret <4 x half> %1
}

define <4 x half> @double_to_v4f16(float, double %a) #0 {
; CHECK-LABEL: double_to_v4f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast double %a to <4 x half>
  ret <4 x half> %1
}










define <8 x i16> @v8f16_to_v8i16(float, <8 x half> %a) #0 {
; CHECK-LABEL: v8f16_to_v8i16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <8 x half> %a to <8 x i16>
  ret <8 x i16> %1
}

define <4 x i32> @v8f16_to_v4i32(float, <8 x half> %a) #0 {
; CHECK-LABEL: v8f16_to_v4i32:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <8 x half> %a to <4 x i32>
  ret <4 x i32> %1
}

define <2 x i64> @v8f16_to_v2i64(float, <8 x half> %a) #0 {
; CHECK-LABEL: v8f16_to_v2i64:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <8 x half> %a to <2 x i64>
  ret <2 x i64> %1
}

define <4 x float> @v8f16_to_v4float(float, <8 x half> %a) #0 {
; CHECK-LABEL: v8f16_to_v4float:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <8 x half> %a to <4 x float>
  ret <4 x float> %1
}

define <2 x double> @v8f16_to_v2double(float, <8 x half> %a) #0 {
; CHECK-LABEL: v8f16_to_v2double:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <8 x half> %a to <2 x double>
  ret <2 x double> %1
}

define <8 x half> @v8i16_to_v8f16(float, <8 x i16> %a) #0 {
; CHECK-LABEL: v8i16_to_v8f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <8 x i16> %a to <8 x half>
  ret <8 x half> %1
}

define <8 x half> @v4i32_to_v8f16(float, <4 x i32> %a) #0 {
; CHECK-LABEL: v4i32_to_v8f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x i32> %a to <8 x half>
  ret <8 x half> %1
}

define <8 x half> @v2i64_to_v8f16(float, <2 x i64> %a) #0 {
; CHECK-LABEL: v2i64_to_v8f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <2 x i64> %a to <8 x half>
  ret <8 x half> %1
}

define <8 x half> @v4float_to_v8f16(float, <4 x float> %a) #0 {
; CHECK-LABEL: v4float_to_v8f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <4 x float> %a to <8 x half>
  ret <8 x half> %1
}

define <8 x half> @v2double_to_v8f16(float, <2 x double> %a) #0 {
; CHECK-LABEL: v2double_to_v8f16:
; CHECK: mov v0.16b, v1.16b
entry:
  %1 = bitcast <2 x double> %a to <8 x half>
  ret <8 x half> %1
}
