; RUN: llc < %s -march=arm64 -aarch64-neon-syntax=apple | FileCheck %s

define i32 @vmin_u8x8(<8 x i8> %a) nounwind ssp {
; CHECK-LABEL: vmin_u8x8:
; CHECK: uminv.8b        b[[REG:[0-9]+]], v0
; CHECK: fmov    [[REG2:w[0-9]+]], s[[REG]]
; CHECK-NOT: and
; CHECK: cbz     [[REG2]],
entry:
  %vminv.i = tail call i32 @llvm.aarch64.neon.uminv.i32.v8i8(<8 x i8> %a) nounwind
  %tmp = trunc i32 %vminv.i to i8
  %tobool = icmp eq i8 %tmp, 0
  br i1 %tobool, label %return, label %if.then

if.then:
  %call1 = tail call i32 bitcast (i32 (...)* @bar to i32 ()*)() nounwind
  br label %return

return:
  %retval.0 = phi i32 [ %call1, %if.then ], [ 0, %entry ]
  ret i32 %retval.0
}

declare i32 @bar(...)

define i32 @vmin_u4x16(<4 x i16> %a) nounwind ssp {
; CHECK-LABEL: vmin_u4x16:
; CHECK: uminv.4h        h[[REG:[0-9]+]], v0
; CHECK: fmov    [[REG2:w[0-9]+]], s[[REG]]
; CHECK-NOT: and
; CHECK: cbz     [[REG2]],
entry:
  %vminv.i = tail call i32 @llvm.aarch64.neon.uminv.i32.v4i16(<4 x i16> %a) nounwind
  %tmp = trunc i32 %vminv.i to i16
  %tobool = icmp eq i16 %tmp, 0
  br i1 %tobool, label %return, label %if.then

if.then:
  %call1 = tail call i32 bitcast (i32 (...)* @bar to i32 ()*)() nounwind
  br label %return

return:
  %retval.0 = phi i32 [ %call1, %if.then ], [ 0, %entry ]
  ret i32 %retval.0
}

define i32 @vmin_u8x16(<8 x i16> %a) nounwind ssp {
; CHECK-LABEL: vmin_u8x16:
; CHECK: uminv.8h        h[[REG:[0-9]+]], v0
; CHECK: fmov    [[REG2:w[0-9]+]], s[[REG]]
; CHECK-NOT: and
; CHECK: cbz     [[REG2]],
entry:
  %vminv.i = tail call i32 @llvm.aarch64.neon.uminv.i32.v8i16(<8 x i16> %a) nounwind
  %tmp = trunc i32 %vminv.i to i16
  %tobool = icmp eq i16 %tmp, 0
  br i1 %tobool, label %return, label %if.then

if.then:
  %call1 = tail call i32 bitcast (i32 (...)* @bar to i32 ()*)() nounwind
  br label %return

return:
  %retval.0 = phi i32 [ %call1, %if.then ], [ 0, %entry ]
  ret i32 %retval.0
}

define i32 @vmin_u16x8(<16 x i8> %a) nounwind ssp {
; CHECK-LABEL: vmin_u16x8:
; CHECK: uminv.16b        b[[REG:[0-9]+]], v0
; CHECK: fmov     [[REG2:w[0-9]+]], s[[REG]]
; CHECK-NOT: and
; CHECK: cbz     [[REG2]],
entry:
  %vminv.i = tail call i32 @llvm.aarch64.neon.uminv.i32.v16i8(<16 x i8> %a) nounwind
  %tmp = trunc i32 %vminv.i to i8
  %tobool = icmp eq i8 %tmp, 0
  br i1 %tobool, label %return, label %if.then

if.then:
  %call1 = tail call i32 bitcast (i32 (...)* @bar to i32 ()*)() nounwind
  br label %return

return:
  %retval.0 = phi i32 [ %call1, %if.then ], [ 0, %entry ]
  ret i32 %retval.0
}

declare i32 @llvm.aarch64.neon.uminv.i32.v16i8(<16 x i8>) nounwind readnone
declare i32 @llvm.aarch64.neon.uminv.i32.v8i16(<8 x i16>) nounwind readnone
declare i32 @llvm.aarch64.neon.uminv.i32.v4i16(<4 x i16>) nounwind readnone
declare i32 @llvm.aarch64.neon.uminv.i32.v8i8(<8 x i8>) nounwind readnone
