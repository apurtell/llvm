; RUN: opt < %s -basicaa -slp-vectorizer -dce -S -mtriple=x86_64-apple-macosx10.8.0 -mcpu=corei7-avx | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.7.0"

@.str = private unnamed_addr constant [6 x i8] c"bingo\00", align 1

; We can't vectorize when the roots are used inside the tree.
;CHECK-LABEL: @in_tree_user(
;CHECK-NOT: load <2 x double>
;CHECK: ret
define void @in_tree_user(double* nocapture %A, i32 %n) {
entry:
  %conv = sitofp i32 %n to double
  br label %for.body

for.body:                                         ; preds = %for.inc, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.inc ]
  %0 = shl nsw i64 %indvars.iv, 1
  %arrayidx = getelementptr inbounds double* %A, i64 %0
  %1 = load double* %arrayidx, align 8
  %mul1 = fmul double %conv, %1
  %mul2 = fmul double %mul1, 7.000000e+00
  %add = fadd double %mul2, 5.000000e+00
  %BadValue = fadd double %add, %add    ; <------------------ In tree user.
  %2 = or i64 %0, 1
  %arrayidx6 = getelementptr inbounds double* %A, i64 %2
  %3 = load double* %arrayidx6, align 8
  %mul8 = fmul double %conv, %3
  %mul9 = fmul double %mul8, 4.000000e+00
  %add10 = fadd double %mul9, 9.000000e+00
  %cmp11 = fcmp ogt double %add, %add10
  br i1 %cmp11, label %if.then, label %for.inc

if.then:                                          ; preds = %for.body
  %call = tail call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([6 x i8]* @.str, i64 0, i64 0))
  br label %for.inc

for.inc:                                          ; preds = %for.body, %if.then
  %indvars.iv.next = add i64 %indvars.iv, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, 100
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %for.inc
  ret void
}

declare i32 @printf(i8* nocapture, ...)

