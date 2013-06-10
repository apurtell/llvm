; RUN: llc -mtriple=x86_64-apple-macosx < %s | FileCheck %s
; rdar://7610418

%ptr = type { i8* }
%struct.s1 = type { %ptr, %ptr }
%struct.s2 = type { i32, i8*, i8*, [256 x %struct.s1*], [8 x i32], i64, i8*, i32, i64, i64, i32, %struct.s3*, %struct.s3*, [49 x i64] }
%struct.s3 = type { %struct.s3*, %struct.s3*, i32, i32, i32 }

define fastcc i8* @t(i32 %base) nounwind {
entry:
; CHECK: t:
; CHECK: leaq (%rax,%rax,4)
  %0 = zext i32 %base to i64
  %1 = getelementptr inbounds %struct.s2* null, i64 %0
  br i1 undef, label %bb1, label %bb2

bb1:
; CHECK: %bb1
; CHECK-NOT: shlq $9
; CHECK-NOT: leaq
; CHECK: call
  %2 = getelementptr inbounds %struct.s2* null, i64 %0, i32 0
  call void @bar(i32* %2) nounwind
  unreachable

bb2:
; CHECK: %bb2
; CHECK-NOT: leaq
; CHECK: callq
  %3 = call fastcc i8* @foo(%struct.s2* %1) nounwind
  unreachable

bb3:
  ret i8* undef
}

declare void @bar(i32*)

declare fastcc i8* @foo(%struct.s2*) nounwind

; rdar://8773371

declare void @printf(...) nounwind

define void @commute(i32 %test_case, i32 %scale) nounwind ssp {
; CHECK: commute:
entry:
  switch i32 %test_case, label %sw.bb307 [
    i32 1, label %sw.bb
    i32 2, label %sw.bb
    i32 3, label %sw.bb
  ]

sw.bb:                                            ; preds = %entry, %entry, %entry
; CHECK: %sw.bb
; CHECK: imull
  %mul = mul nsw i32 %test_case, 3
  %mul20 = mul nsw i32 %mul, %scale
  br i1 undef, label %if.end34, label %sw.bb307

if.end34:                                         ; preds = %sw.bb
; CHECK: %if.end34
; CHECK: leal
; CHECK-NOT: imull
  tail call void (...)* @printf(i32 %test_case, i32 %mul20) nounwind
  %tmp = mul i32 %scale, %test_case
  %tmp752 = mul i32 %tmp, 3
  %tmp753 = zext i32 %tmp752 to i64
  br label %bb.nph743.us

for.body53.us:                                    ; preds = %bb.nph743.us, %for.body53.us
  %exitcond = icmp eq i64 undef, %tmp753
  br i1 %exitcond, label %bb.nph743.us, label %for.body53.us

bb.nph743.us:                                     ; preds = %for.body53.us, %if.end34
  br label %for.body53.us

sw.bb307:                                         ; preds = %sw.bb, %entry
  ret void
}

; CSE physical register defining instruction across MBB boundary.
; rdar://10660865
define i32 @cross_mbb_phys_cse(i32 %a, i32 %b) nounwind ssp {
entry:
; CHECK: cross_mbb_phys_cse:
; CHECK: cmpl
; CHECK: ja
  %cmp = icmp ugt i32 %a, %b
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
; CHECK-NOT: cmpl
; CHECK: sbbl
  %cmp1 = icmp ult i32 %a, %b
  %. = sext i1 %cmp1 to i32
  br label %return

return:                                           ; preds = %if.end, %entry
  %retval.0 = phi i32 [ 1, %entry ], [ %., %if.end ]
  ret i32 %retval.0
}

; rdar://11393714
define i8* @bsd_memchr(i8* %s, i32 %a, i32 %c, i64 %n) nounwind ssp {
; CHECK: %entry
; CHECK: xorl
; CHECK: %preheader
; CHECK: %do.body
; CHECK-NOT: xorl
; CHECK: %do.cond
; CHECK-NOT: xorl
; CHECK: %return
entry:
  %cmp = icmp eq i64 %n, 0
  br i1 %cmp, label %return, label %preheader

preheader:
  %conv2 = and i32 %c, 255
  br label %do.body

do.body:
  %n.addr.0 = phi i64 [ %dec, %do.cond ], [ %n, %preheader ]
  %p.0 = phi i8* [ %incdec.ptr, %do.cond ], [ %s, %preheader ]
  %cmp3 = icmp eq i32 %a, %conv2
  br i1 %cmp3, label %return, label %do.cond

do.cond:
  %incdec.ptr = getelementptr inbounds i8* %p.0, i64 1
  %dec = add i64 %n.addr.0, -1
  %cmp6 = icmp eq i64 %dec, 0
  br i1 %cmp6, label %return, label %do.body

return:
  %retval.0 = phi i8* [ null, %entry ], [ null, %do.cond ], [ %p.0, %do.body ]
  ret i8* %retval.0
}

; PR13578
@t2_global = external global i32

declare i1 @t2_func()

define i32 @t2() {
  store i32 42, i32* @t2_global
  %c = call i1 @t2_func()
  br i1 %c, label %a, label %b

a:
  %l = load i32* @t2_global
  ret i32 %l

b:
  ret i32 0

; CHECK: t2:
; CHECK: t2_global@GOTPCREL(%rip)
; CHECK-NOT: t2_global@GOTPCREL(%rip)
}
