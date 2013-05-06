; Test extensions of f64 to f128.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

; Check register extension.
define void @f1(fp128 *%dst, double %val) {
; CHECK: f1:
; CHECK: lxdbr %f0, %f0
; CHECK: std %f0, 0(%r2)
; CHECK: std %f2, 8(%r2)
; CHECK: br %r14
  %res = fpext double %val to fp128
  store fp128 %res, fp128 *%dst
  ret void
}

; Check the low end of the LXDB range.
define void @f2(fp128 *%dst, double *%ptr) {
; CHECK: f2:
; CHECK: lxdb %f0, 0(%r3)
; CHECK: std %f0, 0(%r2)
; CHECK: std %f2, 8(%r2)
; CHECK: br %r14
  %val = load double *%ptr
  %res = fpext double %val to fp128
  store fp128 %res, fp128 *%dst
  ret void
}

; Check the high end of the aligned LXDB range.
define void @f3(fp128 *%dst, double *%base) {
; CHECK: f3:
; CHECK: lxdb %f0, 4088(%r3)
; CHECK: std %f0, 0(%r2)
; CHECK: std %f2, 8(%r2)
; CHECK: br %r14
  %ptr = getelementptr double *%base, i64 511
  %val = load double *%ptr
  %res = fpext double %val to fp128
  store fp128 %res, fp128 *%dst
  ret void
}

; Check the next doubleword up, which needs separate address logic.
; Other sequences besides this one would be OK.
define void @f4(fp128 *%dst, double *%base) {
; CHECK: f4:
; CHECK: aghi %r3, 4096
; CHECK: lxdb %f0, 0(%r3)
; CHECK: std %f0, 0(%r2)
; CHECK: std %f2, 8(%r2)
; CHECK: br %r14
  %ptr = getelementptr double *%base, i64 512
  %val = load double *%ptr
  %res = fpext double %val to fp128
  store fp128 %res, fp128 *%dst
  ret void
}

; Check negative displacements, which also need separate address logic.
define void @f5(fp128 *%dst, double *%base) {
; CHECK: f5:
; CHECK: aghi %r3, -8
; CHECK: lxdb %f0, 0(%r3)
; CHECK: std %f0, 0(%r2)
; CHECK: std %f2, 8(%r2)
; CHECK: br %r14
  %ptr = getelementptr double *%base, i64 -1
  %val = load double *%ptr
  %res = fpext double %val to fp128
  store fp128 %res, fp128 *%dst
  ret void
}

; Check that LXDB allows indices.
define void @f6(fp128 *%dst, double *%base, i64 %index) {
; CHECK: f6:
; CHECK: sllg %r1, %r4, 3
; CHECK: lxdb %f0, 800(%r1,%r3)
; CHECK: std %f0, 0(%r2)
; CHECK: std %f2, 8(%r2)
; CHECK: br %r14
  %ptr1 = getelementptr double *%base, i64 %index
  %ptr2 = getelementptr double *%ptr1, i64 100
  %val = load double *%ptr2
  %res = fpext double %val to fp128
  store fp128 %res, fp128 *%dst
  ret void
}
