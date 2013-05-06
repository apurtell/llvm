; Test 64-bit atomic exchange.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

; Check register exchange.
define i64 @f1(i64 %dummy, i64 *%src, i64 %b) {
; CHECK: f1:
; CHECK: lg %r2, 0(%r3)
; CHECK: [[LABEL:\.[^:]*]]:
; CHECK: csg %r2, %r4, 0(%r3)
; CHECK: j{{g?}}lh [[LABEL]]
; CHECK: br %r14
  %res = atomicrmw xchg i64 *%src, i64 %b seq_cst
  ret i64 %res
}

; Check the high end of the aligned CSG range.
define i64 @f2(i64 %dummy, i64 *%src, i64 %b) {
; CHECK: f2:
; CHECK: lg %r2, 524280(%r3)
; CHECK: csg %r2, {{%r[0-9]+}}, 524280(%r3)
; CHECK: br %r14
  %ptr = getelementptr i64 *%src, i64 65535
  %res = atomicrmw xchg i64 *%ptr, i64 %b seq_cst
  ret i64 %res
}

; Check the next doubleword up, which requires separate address logic.
define i64 @f3(i64 %dummy, i64 *%src, i64 %b) {
; CHECK: f3:
; CHECK: agfi %r3, 524288
; CHECK: lg %r2, 0(%r3)
; CHECK: csg %r2, {{%r[0-9]+}}, 0(%r3)
; CHECK: br %r14
  %ptr = getelementptr i64 *%src, i64 65536
  %res = atomicrmw xchg i64 *%ptr, i64 %b seq_cst
  ret i64 %res
}

; Check the low end of the CSG range.
define i64 @f4(i64 %dummy, i64 *%src, i64 %b) {
; CHECK: f4:
; CHECK: lg %r2, -524288(%r3)
; CHECK: csg %r2, {{%r[0-9]+}}, -524288(%r3)
; CHECK: br %r14
  %ptr = getelementptr i64 *%src, i64 -65536
  %res = atomicrmw xchg i64 *%ptr, i64 %b seq_cst
  ret i64 %res
}

; Check the next doubleword down, which requires separate address logic.
define i64 @f5(i64 %dummy, i64 *%src, i64 %b) {
; CHECK: f5:
; CHECK: agfi %r3, -524296
; CHECK: lg %r2, 0(%r3)
; CHECK: csg %r2, {{%r[0-9]+}}, 0(%r3)
; CHECK: br %r14
  %ptr = getelementptr i64 *%src, i64 -65537
  %res = atomicrmw xchg i64 *%ptr, i64 %b seq_cst
  ret i64 %res
}

; Check that indexed addresses are not allowed.
define i64 @f6(i64 %dummy, i64 %base, i64 %index, i64 %b) {
; CHECK: f6:
; CHECK: agr %r3, %r4
; CHECK: lg %r2, 0(%r3)
; CHECK: csg %r2, {{%r[0-9]+}}, 0(%r3)
; CHECK: br %r14
  %add = add i64 %base, %index
  %ptr = inttoptr i64 %add to i64 *
  %res = atomicrmw xchg i64 *%ptr, i64 %b seq_cst
  ret i64 %res
}

; Check exchange of a constant.  We should force it into a register and
; use the sequence above.
define i64 @f7(i64 %dummy, i64 *%ptr) {
; CHECK: f7:
; CHECK: llilf [[VALUE:%r[0-9+]]], 3000000000
; CHECK: lg %r2, 0(%r3)
; CHECK: [[LABEL:\.[^:]*]]:
; CHECK: csg %r2, [[VALUE]], 0(%r3)
; CHECK: j{{g?}}lh [[LABEL]]
; CHECK: br %r14
  %res = atomicrmw xchg i64 *%ptr, i64 3000000000 seq_cst
  ret i64 %res
}
