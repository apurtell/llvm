; Test additions between an i64 and a zero-extended i32.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

; Check ALGFR.
define i64 @f1(i64 %a, i32 %b) {
; CHECK: f1:
; CHECK: algfr %r2, %r3
; CHECK: br %r14
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check ALGF with no displacement.
define i64 @f2(i64 %a, i32 *%src) {
; CHECK: f2:
; CHECK: algf %r2, 0(%r3)
; CHECK: br %r14
  %b = load i32 *%src
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check the high end of the aligned ALGF range.
define i64 @f3(i64 %a, i32 *%src) {
; CHECK: f3:
; CHECK: algf %r2, 524284(%r3)
; CHECK: br %r14
  %ptr = getelementptr i32 *%src, i64 131071
  %b = load i32 *%ptr
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check the next word up, which needs separate address logic.
; Other sequences besides this one would be OK.
define i64 @f4(i64 %a, i32 *%src) {
; CHECK: f4:
; CHECK: agfi %r3, 524288
; CHECK: algf %r2, 0(%r3)
; CHECK: br %r14
  %ptr = getelementptr i32 *%src, i64 131072
  %b = load i32 *%ptr
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check the high end of the negative aligned ALGF range.
define i64 @f5(i64 %a, i32 *%src) {
; CHECK: f5:
; CHECK: algf %r2, -4(%r3)
; CHECK: br %r14
  %ptr = getelementptr i32 *%src, i64 -1
  %b = load i32 *%ptr
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check the low end of the ALGF range.
define i64 @f6(i64 %a, i32 *%src) {
; CHECK: f6:
; CHECK: algf %r2, -524288(%r3)
; CHECK: br %r14
  %ptr = getelementptr i32 *%src, i64 -131072
  %b = load i32 *%ptr
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check the next word down, which needs separate address logic.
; Other sequences besides this one would be OK.
define i64 @f7(i64 %a, i32 *%src) {
; CHECK: f7:
; CHECK: agfi %r3, -524292
; CHECK: algf %r2, 0(%r3)
; CHECK: br %r14
  %ptr = getelementptr i32 *%src, i64 -131073
  %b = load i32 *%ptr
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}

; Check that ALGF allows an index.
define i64 @f8(i64 %a, i64 %src, i64 %index) {
; CHECK: f8:
; CHECK: algf %r2, 524284({{%r4,%r3|%r3,%r4}})
; CHECK: br %r14
  %add1 = add i64 %src, %index
  %add2 = add i64 %add1, 524284
  %ptr = inttoptr i64 %add2 to i32 *
  %b = load i32 *%ptr
  %bext = zext i32 %b to i64
  %add = add i64 %a, %bext
  ret i64 %add
}
