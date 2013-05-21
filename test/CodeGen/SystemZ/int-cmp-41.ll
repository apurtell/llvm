; Test 64-bit comparisons in which the second operand is sign-extended
; from a PC-relative i32.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

@g = global i32 1

; Check signed comparison.
define i64 @f1(i64 %src1) {
; CHECK: f1:
; CHECK: cgfrl %r2, g
; CHECK-NEXT: jl
; CHECK: br %r14
entry:
  %val = load i32 *@g
  %src2 = sext i32 %val to i64
  %cond = icmp slt i64 %src1, %src2
  br i1 %cond, label %exit, label %mulb
mulb:
  %mul = mul i64 %src1, %src1
  br label %exit
exit:
  %res = phi i64 [ %src1, %entry ], [ %mul, %mulb ]
  ret i64 %res
}

; Check unsigned comparison, which cannot use CHRL.
define i64 @f2(i64 %src1) {
; CHECK: f2:
; CHECK-NOT: cgfrl
; CHECK: br %r14
entry:
  %val = load i32 *@g
  %src2 = sext i32 %val to i64
  %cond = icmp ult i64 %src1, %src2
  br i1 %cond, label %exit, label %mulb
mulb:
  %mul = mul i64 %src1, %src1
  br label %exit
exit:
  %res = phi i64 [ %src1, %entry ], [ %mul, %mulb ]
  ret i64 %res
}

; Check equality.
define i64 @f3(i64 %src1) {
; CHECK: f3:
; CHECK: cgfrl %r2, g
; CHECK-NEXT: je
; CHECK: br %r14
entry:
  %val = load i32 *@g
  %src2 = sext i32 %val to i64
  %cond = icmp eq i64 %src1, %src2
  br i1 %cond, label %exit, label %mulb
mulb:
  %mul = mul i64 %src1, %src1
  br label %exit
exit:
  %res = phi i64 [ %src1, %entry ], [ %mul, %mulb ]
  ret i64 %res
}

; Check inequality.
define i64 @f4(i64 %src1) {
; CHECK: f4:
; CHECK: cgfrl %r2, g
; CHECK-NEXT: jlh
; CHECK: br %r14
entry:
  %val = load i32 *@g
  %src2 = sext i32 %val to i64
  %cond = icmp ne i64 %src1, %src2
  br i1 %cond, label %exit, label %mulb
mulb:
  %mul = mul i64 %src1, %src1
  br label %exit
exit:
  %res = phi i64 [ %src1, %entry ], [ %mul, %mulb ]
  ret i64 %res
}
