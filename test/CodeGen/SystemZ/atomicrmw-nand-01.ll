; Test 8-bit atomic NANDs.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-SHIFT1
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-SHIFT2

; Check NAND of a variable.
; - CHECK is for the main loop.
; - CHECK-SHIFT1 makes sure that the negated shift count used by the second
;   RLL is set up correctly.  The negation is independent of the NILL and L
;   tested in CHECK.
; - CHECK-SHIFT2 makes sure that %b is shifted into the high part of the word
;   before being used, and that the low bits are set to 1.  This sequence is
;   independent of the other loop prologue instructions.
define i8 @f1(i8 *%src, i8 %b) {
; CHECK: f1:
; CHECK: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK: nill %r2, 65532
; CHECK: l [[OLD:%r[0-9]+]], 0(%r2)
; CHECK: [[LABEL:\.[^:]*]]:
; CHECK: rll [[ROT:%r[0-9]+]], [[OLD]], 0([[SHIFT]])
; CHECK: nr [[ROT]], %r3
; CHECK: xilf [[ROT]], 4278190080
; CHECK: rll [[NEW:%r[0-9]+]], [[ROT]], 0({{%r[1-9]+}})
; CHECK: cs [[OLD]], [[NEW]], 0(%r2)
; CHECK: j{{g?}}lh [[LABEL]]
; CHECK: rll %r2, [[OLD]], 8([[SHIFT]])
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
; CHECK-SHIFT2: sll %r3, 24
; CHECK-SHIFT2: oilf %r3, 16777215
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: nr {{%r[0-9]+}}, %r3
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: rll
; CHECK-SHIFT2: br %r14
  %res = atomicrmw nand i8 *%src, i8 %b seq_cst
  ret i8 %res
}

; Check the minimum signed value.  We AND the rotated word with 0x80ffffff.
define i8 @f2(i8 *%src) {
; CHECK: f2:
; CHECK: sllg [[SHIFT:%r[1-9]+]], %r2, 3
; CHECK: nill %r2, 65532
; CHECK: l [[OLD:%r[0-9]+]], 0(%r2)
; CHECK: [[LABEL:\.[^:]*]]:
; CHECK: rll [[ROT:%r[0-9]+]], [[OLD]], 0([[SHIFT]])
; CHECK: nilh [[ROT]], 33023
; CHECK: xilf [[ROT]], 4278190080
; CHECK: rll [[NEW:%r[0-9]+]], [[ROT]], 0([[NEGSHIFT:%r[1-9]+]])
; CHECK: cs [[OLD]], [[NEW]], 0(%r2)
; CHECK: j{{g?}}lh [[LABEL]]
; CHECK: rll %r2, [[OLD]], 8([[SHIFT]])
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
; CHECK-SHIFT2: br %r14
  %res = atomicrmw nand i8 *%src, i8 -128 seq_cst
  ret i8 %res
}

; Check NANDs of -2 (-1 isn't useful).  We AND the rotated word with 0xfeffffff.
define i8 @f3(i8 *%src) {
; CHECK: f3:
; CHECK: nilh [[ROT]], 65279
; CHECK: xilf [[ROT]], 4278190080
; CHECK: br %r14
;
; CHECK-SHIFT1: f3:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f3:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw nand i8 *%src, i8 -2 seq_cst
  ret i8 %res
}

; Check NANDs of 1.  We AND the rotated word with 0x01ffffff.
define i8 @f4(i8 *%src) {
; CHECK: f4:
; CHECK: nilh [[ROT]], 511
; CHECK: xilf [[ROT]], 4278190080
; CHECK: br %r14
;
; CHECK-SHIFT1: f4:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f4:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw nand i8 *%src, i8 1 seq_cst
  ret i8 %res
}

; Check the maximum signed value.  We AND the rotated word with 0x7fffffff.
define i8 @f5(i8 *%src) {
; CHECK: f5:
; CHECK: nilh [[ROT]], 32767
; CHECK: xilf [[ROT]], 4278190080
; CHECK: br %r14
;
; CHECK-SHIFT1: f5:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f5:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw nand i8 *%src, i8 127 seq_cst
  ret i8 %res
}

; Check NANDs of a large unsigned value.  We AND the rotated word with
; 0xfdffffff.
define i8 @f6(i8 *%src) {
; CHECK: f6:
; CHECK: nilh [[ROT]], 65023
; CHECK: xilf [[ROT]], 4278190080
; CHECK: br %r14
;
; CHECK-SHIFT1: f6:
; CHECK-SHIFT1: br %r14
; CHECK-SHIFT2: f6:
; CHECK-SHIFT2: br %r14
  %res = atomicrmw nand i8 *%src, i8 253 seq_cst
  ret i8 %res
}
