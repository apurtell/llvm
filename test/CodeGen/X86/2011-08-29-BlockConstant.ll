; RUN: llc -march=x86-64 < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@x = global [500 x i64] zeroinitializer, align 64 ; <[500 x i64]*>
; CHECK: x:
; CHECK: .zero	4000

@y = global [63 x i64] [
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262,
  i64 6799976246779207262, i64 6799976246779207262, i64 6799976246779207262],
  align 64 ; <[63 x i64]*> 0x5e5e5e5e
; CHECK: y:
; CHECK: .zero	504,94
