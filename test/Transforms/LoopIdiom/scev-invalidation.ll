; RUN: opt -S -indvars -loop-idiom < %s
; PR14214
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @quote_arg() nounwind {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %backslashes.0 = phi i32 [ undef, %entry ], [ %backslashes.2, %for.inc ]
  %p.0 = phi i8* [ undef, %entry ], [ %incdec.ptr3, %for.inc ]
  %q.0 = phi i8* [ undef, %entry ], [ %q.2, %for.inc ]
  %0 = load i8* %p.0, align 1
  switch i8 %0, label %while.cond.preheader [
    i8 0, label %for.cond4.preheader
    i8 92, label %for.inc
  ]

while.cond.preheader:                             ; preds = %for.cond
  %tobool210 = icmp eq i32 %backslashes.0, 0
  br i1 %tobool210, label %for.inc.loopexit, label %while.body.lr.ph

while.body.lr.ph:                                 ; preds = %while.cond.preheader
  %1 = add i32 %backslashes.0, -1
  %2 = zext i32 %1 to i64
  br label %while.body

for.cond4.preheader:                              ; preds = %for.cond
  %tobool57 = icmp eq i32 %backslashes.0, 0
  br i1 %tobool57, label %for.end10, label %for.body6.lr.ph

for.body6.lr.ph:                                  ; preds = %for.cond4.preheader
  br label %for.body6

while.body:                                       ; preds = %while.body.lr.ph, %while.body
  %q.112 = phi i8* [ %q.0, %while.body.lr.ph ], [ %incdec.ptr, %while.body ]
  %backslashes.111 = phi i32 [ %backslashes.0, %while.body.lr.ph ], [ %dec, %while.body ]
  %incdec.ptr = getelementptr inbounds i8, i8* %q.112, i64 1
  store i8 92, i8* %incdec.ptr, align 1
  %dec = add nsw i32 %backslashes.111, -1
  %tobool2 = icmp eq i32 %dec, 0
  br i1 %tobool2, label %while.cond.for.inc.loopexit_crit_edge, label %while.body

while.cond.for.inc.loopexit_crit_edge:            ; preds = %while.body
  %scevgep.sum = add i64 %2, 1
  %scevgep13 = getelementptr i8, i8* %q.0, i64 %scevgep.sum
  br label %for.inc.loopexit

for.inc.loopexit:                                 ; preds = %while.cond.for.inc.loopexit_crit_edge, %while.cond.preheader
  %q.1.lcssa = phi i8* [ %scevgep13, %while.cond.for.inc.loopexit_crit_edge ], [ %q.0, %while.cond.preheader ]
  br label %for.inc

for.inc:                                          ; preds = %for.inc.loopexit, %for.cond
  %backslashes.2 = phi i32 [ %backslashes.0, %for.cond ], [ 0, %for.inc.loopexit ]
  %q.2 = phi i8* [ %q.0, %for.cond ], [ %q.1.lcssa, %for.inc.loopexit ]
  %incdec.ptr3 = getelementptr inbounds i8, i8* %p.0, i64 1
  br label %for.cond

for.body6:                                        ; preds = %for.body6.lr.ph, %for.body6
  %q.39 = phi i8* [ %q.0, %for.body6.lr.ph ], [ %incdec.ptr7, %for.body6 ]
  %backslashes.38 = phi i32 [ %backslashes.0, %for.body6.lr.ph ], [ %dec9, %for.body6 ]
  %incdec.ptr7 = getelementptr inbounds i8, i8* %q.39, i64 1
  store i8 92, i8* %incdec.ptr7, align 1
  %dec9 = add nsw i32 %backslashes.38, -1
  %tobool5 = icmp eq i32 %dec9, 0
  br i1 %tobool5, label %for.cond4.for.end10_crit_edge, label %for.body6

for.cond4.for.end10_crit_edge:                    ; preds = %for.body6
  br label %for.end10

for.end10:                                        ; preds = %for.cond4.for.end10_crit_edge, %for.cond4.preheader
  ret i32 undef
}
