; Test 32-bit arithmetic shifts right.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

; Check the low end of the SRA range.
define i32 @f1(i32 %a) {
; CHECK: f1:
; CHECK: sra %r2, 1
; CHECK: br %r14
  %shift = ashr i32 %a, 1
  ret i32 %shift
}

; Check the high end of the defined SRA range.
define i32 @f2(i32 %a) {
; CHECK: f2:
; CHECK: sra %r2, 31
; CHECK: br %r14
  %shift = ashr i32 %a, 31
  ret i32 %shift
}

; We don't generate shifts by out-of-range values.
define i32 @f3(i32 %a) {
; CHECK: f3:
; CHECK-NOT: sra %r2, 32
; CHECK: br %r14
  %shift = ashr i32 %a, 32
  ret i32 %shift
}

; Make sure that we don't generate negative shift amounts.
define i32 @f4(i32 %a, i32 %amt) {
; CHECK: f4:
; CHECK-NOT: sra %r2, -1{{.*}}
; CHECK: br %r14
  %sub = sub i32 %amt, 1
  %shift = ashr i32 %a, %sub
  ret i32 %shift
}

; Check variable shifts.
define i32 @f5(i32 %a, i32 %amt) {
; CHECK: f5:
; CHECK: sra %r2, 0(%r3)
; CHECK: br %r14
  %shift = ashr i32 %a, %amt
  ret i32 %shift
}

; Check shift amounts that have a constant term.
define i32 @f6(i32 %a, i32 %amt) {
; CHECK: f6:
; CHECK: sra %r2, 10(%r3)
; CHECK: br %r14
  %add = add i32 %amt, 10
  %shift = ashr i32 %a, %add
  ret i32 %shift
}

; ...and again with a truncated 64-bit shift amount.
define i32 @f7(i32 %a, i64 %amt) {
; CHECK: f7:
; CHECK: sra %r2, 10(%r3)
; CHECK: br %r14
  %add = add i64 %amt, 10
  %trunc = trunc i64 %add to i32
  %shift = ashr i32 %a, %trunc
  ret i32 %shift
}

; Check shift amounts that have the largest in-range constant term.  We could
; mask the amount instead.
define i32 @f8(i32 %a, i32 %amt) {
; CHECK: f8:
; CHECK: sra %r2, 4095(%r3)
; CHECK: br %r14
  %add = add i32 %amt, 4095
  %shift = ashr i32 %a, %add
  ret i32 %shift
}

; Check the next value up.  Again, we could mask the amount instead.
define i32 @f9(i32 %a, i32 %amt) {
; CHECK: f9:
; CHECK: ahi %r3, 4096
; CHECK: sra %r2, 0(%r3)
; CHECK: br %r14
  %add = add i32 %amt, 4096
  %shift = ashr i32 %a, %add
  ret i32 %shift
}

; Check that we don't try to generate "indexed" shifts.
define i32 @f10(i32 %a, i32 %b, i32 %c) {
; CHECK: f10:
; CHECK: ar {{%r3, %r4|%r4, %r3}}
; CHECK: sra %r2, 0({{%r[34]}})
; CHECK: br %r14
  %add = add i32 %b, %c
  %shift = ashr i32 %a, %add
  ret i32 %shift
}

; Check that the shift amount uses an address register.  It cannot be in %r0.
define i32 @f11(i32 %a, i32 *%ptr) {
; CHECK: f11:
; CHECK: l %r1, 0(%r3)
; CHECK: sra %r2, 0(%r1)
; CHECK: br %r14
  %amt = load i32 *%ptr
  %shift = ashr i32 %a, %amt
  ret i32 %shift
}
