; RUN: llc < %s  -mtriple=arm-linux-gnueabi -regalloc=linearscan | FileCheck %s

; This test depends on linear scan's reserved register coalescing.

@.str = private constant [1 x i8] zeroinitializer, align 1

define void @g() {
entry:
;CHECK: [sp, #8]
;CHECK: [sp, #12]
;CHECK: [sp]
        tail call  void (i8*, ...)* @f(i8* getelementptr ([1 x i8]* @.str, i32 0, i32 0), i32 1, double 2.000000e+00, i32 3, double 4.000000e+00)
        ret void
}

declare void @f(i8*, ...)
