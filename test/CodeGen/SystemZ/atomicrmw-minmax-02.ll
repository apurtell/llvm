; Test 8-bit atomic min/max operations.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-SHIFT1
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-SHIFT2

; Check signed minimum.
; - CHECK is for the main loop.
; - CHECK-SHIFT1 makes sure that the negated shift count used by the second
;   RLL is set up correctly.  The negation is independent of the NILL and L
;   tested in CHECK.
; - CHECK-SHIFT2 makes sure that %b is shifted into the high part of the word
;   before being used, and that the low bits are set to 1.  This sequence is
;   independent of the other loop prologue instructions.
define i16 @f1(i16 *%src, i16 %b) {
; CHECK: f1:
; CHECK: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK: nill %r2, 65532
; CHECK: l [[OLD:%r[0-9]+]], 0(%r2)
; CHECK: [[LOOP:\.[^:]*]]:
; CHECK: rll [[ROT:%r[0-9]+]], [[OLD]], 0([[SHIFT]])
; CHECK: cr [[ROT]], %r3
; CHECK: j{{g?}}le [[KEEP:\..*]]
; CHECK: risbg [[ROT]], %r3, 32, 47, 0
; CHECK: [[KEEP]]:
; CHECK: rll [[NEW:%r[0-9]+]], [[ROT]], 0({{%r[1-9]+}})
; CHECK: cs [[OLD]], [[NEW]], 0(%r2)
; CHECK: j{{g?}}lh [[LOOP]]
; CHECK: rll %r2, [[OLD]], 16([[SHIFT]])
; CHECK: br %r14
;
; CHECK-SHIFT1: f1:
; CHECK-SHIFT1: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK-SHIFT1: lcr [[NEGSHIFT:%r[1-9]+]], [[SHIFT]]
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: rll {{%r[0-9]+}}, {{%r[0-9]+}}, 0([[NEGSHIFT]])
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: br %r14
;
; CHECK-SHIFT2: f1:
; CHECK-SHIFT2: sll %r3, 16
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: cr {{%r[0-9]+}}, %r3
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: br %r14
  %res = atomicrmw min i16 *%src, i16 %b seq_cst
  ret i16 %res
}

; Check signed maximum.
define i16 @f2(i16 *%src, i16 %b) {
; CHECK: f2:
; CHECK: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK: nill %r2, 65532
; CHECK: l [[OLD:%r[0-9]+]], 0(%r2)
; CHECK: [[LOOP:\.[^:]*]]:
; CHECK: rll [[ROT:%r[0-9]+]], [[OLD]], 0([[SHIFT]])
; CHECK: cr [[ROT]], %r3
; CHECK: j{{g?}}he [[KEEP:\..*]]
; CHECK: risbg [[ROT]], %r3, 32, 47, 0
; CHECK: [[KEEP]]:
; CHECK: rll [[NEW:%r[0-9]+]], [[ROT]], 0({{%r[1-9]+}})
; CHECK: cs [[OLD]], [[NEW]], 0(%r2)
; CHECK: j{{g?}}lh [[LOOP]]
; CHECK: rll %r2, [[OLD]], 16([[SHIFT]])
; CHECK: br %r14
;
; CHECK-SHIFT1: f2:
; CHECK-SHIFT1: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK-SHIFT1: lcr [[NEGSHIFT:%r[1-9]+]], [[SHIFT]]
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: rll {{%r[0-9]+}}, {{%r[0-9]+}}, 0([[NEGSHIFT]])
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: br %r14
;
; CHECK-SHIFT2: f2:
; CHECK-SHIFT2: sll %r3, 16
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: cr {{%r[0-9]+}}, %r3
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: br %r14
  %res = atomicrmw max i16 *%src, i16 %b seq_cst
  ret i16 %res
}

; Check unsigned minimum.
define i16 @f3(i16 *%src, i16 %b) {
; CHECK: f3:
; CHECK: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK: nill %r2, 65532
; CHECK: l [[OLD:%r[0-9]+]], 0(%r2)
; CHECK: [[LOOP:\.[^:]*]]:
; CHECK: rll [[ROT:%r[0-9]+]], [[OLD]], 0([[SHIFT]])
; CHECK: clr [[ROT]], %r3
; CHECK: j{{g?}}le [[KEEP:\..*]]
; CHECK: risbg [[ROT]], %r3, 32, 47, 0
; CHECK: [[KEEP]]:
; CHECK: rll [[NEW:%r[0-9]+]], [[ROT]], 0({{%r[1-9]+}})
; CHECK: cs [[OLD]], [[NEW]], 0(%r2)
; CHECK: j{{g?}}lh [[LOOP]]
; CHECK: rll %r2, [[OLD]], 16([[SHIFT]])
; CHECK: br %r14
;
; CHECK-SHIFT1: f3:
; CHECK-SHIFT1: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK-SHIFT1: lcr [[NEGSHIFT:%r[1-9]+]], [[SHIFT]]
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: rll {{%r[0-9]+}}, {{%r[0-9]+}}, 0([[NEGSHIFT]])
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: br %r14
;
; CHECK-SHIFT2: f3:
; CHECK-SHIFT2: sll %r3, 16
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: clr {{%r[0-9]+}}, %r3
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: br %r14
  %res = atomicrmw umin i16 *%src, i16 %b seq_cst
  ret i16 %res
}

; Check unsigned maximum.
define i16 @f4(i16 *%src, i16 %b) {
; CHECK: f4:
; CHECK: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK: nill %r2, 65532
; CHECK: l [[OLD:%r[0-9]+]], 0(%r2)
; CHECK: [[LOOP:\.[^:]*]]:
; CHECK: rll [[ROT:%r[0-9]+]], [[OLD]], 0([[SHIFT]])
; CHECK: clr [[ROT]], %r3
; CHECK: j{{g?}}he [[KEEP:\..*]]
; CHECK: risbg [[ROT]], %r3, 32, 47, 0
; CHECK: [[KEEP]]:
; CHECK: rll [[NEW:%r[0-9]+]], [[ROT]], 0({{%r[1-9]+}})
; CHECK: cs [[OLD]], [[NEW]], 0(%r2)
; CHECK: j{{g?}}lh [[LOOP]]
; CHECK: rll %r2, [[OLD]], 16([[SHIFT]])
; CHECK: br %r14
;
; CHECK-SHIFT1: f4:
; CHECK-SHIFT1: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK-SHIFT1: lcr [[NEGSHIFT:%r[1-9]+]], [[SHIFT]]
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: rll {{%r[0-9]+}}, {{%r[0-9]+}}, 0([[NEGSHIFT]])
; CHECK-SHIFT1: rll
; CHECK-SHIFT1: br %r14
;
; CHECK-SHIFT2: f4:
; CHECK-SHIFT2: sll %r3, 16
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: clr {{%r[0-9]+}}, %r3
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: br %r14
  %res = atomicrmw umax i16 *%src, i16 %b seq_cst
  ret i16 %res
}

; Check the lowest useful signed minimum value.  We need to load 0x80010000
; into the source register.
define i16 @f5(i16 *%src) {
; CHECK: f5:
; CHECK: llilh [[SRC2:%r[0-9]+]], 32769
; CHECK: cr [[ROT:%r[0-9]+]], [[SRC2]]
; CHECK: risbg [[ROT]], [[SRC2]], 32, 47, 0
; CHECK: br %r14
;
; CHECK-SHIFT1: f5:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f5:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw min i16 *%src, i16 -32767 seq_cst
  ret i16 %res
}

; Check the highest useful signed maximum value.  We need to load 0x7ffe0000
; into the source register.
define i16 @f6(i16 *%src) {
; CHECK: f6:
; CHECK: llilh [[SRC2:%r[0-9]+]], 32766
; CHECK: cr [[ROT:%r[0-9]+]], [[SRC2]]
; CHECK: risbg [[ROT]], [[SRC2]], 32, 47, 0
; CHECK: br %r14
;
; CHECK-SHIFT1: f6:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f6:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw max i16 *%src, i16 32766 seq_cst
  ret i16 %res
}

; Check the lowest useful unsigned maximum value.  We need to load 0x00010000
; into the source register.
define i16 @f7(i16 *%src) {
; CHECK: f7:
; CHECK: llilh [[SRC2:%r[0-9]+]], 1
; CHECK: clr [[ROT:%r[0-9]+]], [[SRC2]]
; CHECK: risbg [[ROT]], [[SRC2]], 32, 47, 0
; CHECK: br %r14
;
; CHECK-SHIFT1: f7:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f7:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw umin i16 *%src, i16 1 seq_cst
  ret i16 %res
}

; Check the highest useful unsigned maximum value.  We need to load 0xfffe0000
; into the source register.
define i16 @f8(i16 *%src) {
; CHECK: f8:
; CHECK: llilh [[SRC2:%r[0-9]+]], 65534
; CHECK: clr [[ROT:%r[0-9]+]], [[SRC2]]
; CHECK: risbg [[ROT]], [[SRC2]], 32, 47, 0
; CHECK: br %r14
;
; CHECK-SHIFT1: f8:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f8:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw umax i16 *%src, i16 65534 seq_cst
  ret i16 %res
}
