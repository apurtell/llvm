; RUN: llc < %s -march=arm64 | FileCheck %s
define i64 @normal_load(i64* nocapture %bar) nounwind readonly {
; CHECK: normal_load
; CHECK: ldp
; CHECK-NEXT: add
; CHECK-NEXT: ret
  %add.ptr = getelementptr inbounds i64* %bar, i64 1
  %tmp = load i64* %add.ptr, align 8
  %add.ptr1 = getelementptr inbounds i64* %bar, i64 2
  %tmp1 = load i64* %add.ptr1, align 8
  %add = add nsw i64 %tmp1, %tmp
  ret i64 %add
}

define i64 @volatile_load(i64* nocapture %bar) nounwind {
; CHECK: volatile_load
; CHECK: ldr
; CHECK-NEXT: ldr
; CHECK-NEXT: add
; CHECK-NEXT: ret
  %add.ptr = getelementptr inbounds i64* %bar, i64 1
  %tmp = load volatile i64* %add.ptr, align 8
  %add.ptr1 = getelementptr inbounds i64* %bar, i64 2
  %tmp1 = load volatile i64* %add.ptr1, align 8
  %add = add nsw i64 %tmp1, %tmp
  ret i64 %add
}
