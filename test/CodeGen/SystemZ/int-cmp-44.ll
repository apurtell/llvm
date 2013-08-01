; Test that compares are ommitted if CC already has the right value
; (z10 version).
;
; RUN: llc < %s -mtriple=s390x-linux-gnu -mcpu=z10 | FileCheck %s

declare void @foo()

; Addition provides enough for equality comparisons with zero.  First teest
; the EQ case.
define i32 @f1(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f1:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: je .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  %cmp = icmp eq i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and again with NE.
define i32 @f2(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f2:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: jne .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; SLT requires a comparison.
define i32 @f3(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f3:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: cijl %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  %cmp = icmp slt i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...SLE too.
define i32 @f4(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f4:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: cijle %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  %cmp = icmp sle i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...SGT too.
define i32 @f5(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f5:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: cijh %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  %cmp = icmp sgt i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...SGE too.
define i32 @f6(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f6:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: cijhe %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  %cmp = icmp sge i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; Subtraction also provides enough for equality comparisons with zero.
define i32 @f7(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f7:
; CHECK: s %r2, 0(%r4)
; CHECK-NEXT: jne .L{{.*}}
; CHECK: br %r14
entry:
  %cur = load i32 *%dest
  %res = sub i32 %a, %cur
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...but not for ordered comparisons.
define i32 @f8(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f8:
; CHECK: s %r2, 0(%r4)
; CHECK-NEXT: cijl %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %cur = load i32 *%dest
  %res = sub i32 %a, %cur
  %cmp = icmp slt i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; Logic register-register instructions also provide enough for equality
; comparisons with zero.
define i32 @f9(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f9:
; CHECK: nr %r2, %r3
; CHECK-NEXT: jl .L{{.*}}
; CHECK: br %r14
entry:
  %res = and i32 %a, %b
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...but not for ordered comparisons.
define i32 @f10(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f10:
; CHECK: nr %r2, %r3
; CHECK-NEXT: cijl %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %res = and i32 %a, %b
  %cmp = icmp slt i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; Logic register-immediate instructions also provide enough for equality
; comparisons with zero if the immediate covers the whole register.
define i32 @f11(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f11:
; CHECK: nilf %r2, 100
; CHECK-NEXT: jl .L{{.*}}
; CHECK: br %r14
entry:
  %res = and i32 %a, 100
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; Partial logic register-immediate instructions do not provide simple
; zero results.
define i32 @f12(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f12:
; CHECK: nill %r2, 65436
; CHECK-NEXT: cijlh %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %res = and i32 %a, -100
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; SRA provides the same CC result as a comparison with zero.
define i32 @f13(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f13:
; CHECK: sra %r2, 0(%r3)
; CHECK-NEXT: je .L{{.*}}
; CHECK: br %r14
entry:
  %res = ashr i32 %a, %b
  %cmp = icmp eq i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and again with NE.
define i32 @f14(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f14:
; CHECK: sra %r2, 0(%r3)
; CHECK-NEXT: jlh .L{{.*}}
; CHECK: br %r14
entry:
  %res = ashr i32 %a, %b
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and SLT.
define i32 @f15(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f15:
; CHECK: sra %r2, 0(%r3)
; CHECK-NEXT: jl .L{{.*}}
; CHECK: br %r14
entry:
  %res = ashr i32 %a, %b
  %cmp = icmp slt i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and SLE.
define i32 @f16(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f16:
; CHECK: sra %r2, 0(%r3)
; CHECK-NEXT: jle .L{{.*}}
; CHECK: br %r14
entry:
  %res = ashr i32 %a, %b
  %cmp = icmp sle i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and SGT.
define i32 @f17(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f17:
; CHECK: sra %r2, 0(%r3)
; CHECK-NEXT: jh .L{{.*}}
; CHECK: br %r14
entry:
  %res = ashr i32 %a, %b
  %cmp = icmp sgt i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and SGE.
define i32 @f18(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f18:
; CHECK: sra %r2, 0(%r3)
; CHECK-NEXT: jhe .L{{.*}}
; CHECK: br %r14
entry:
  %res = ashr i32 %a, %b
  %cmp = icmp sge i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; RISBG provides the same result as a comparison against zero.
; Test the EQ case.
define i64 @f19(i64 %a, i64 %b, i64 *%dest) {
; CHECK-LABEL: f19:
; CHECK: risbg %r2, %r3, 0, 190, 0
; CHECK-NEXT: je .L{{.*}}
; CHECK: br %r14
entry:
  %res = and i64 %b, -2
  %cmp = icmp eq i64 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i64 %b, i64 *%dest
  br label %exit

exit:
  ret i64 %res
}

; ...and the SLT case.
define i64 @f20(i64 %a, i64 %b, i64 *%dest) {
; CHECK-LABEL: f20:
; CHECK: risbg %r2, %r3, 0, 190, 0
; CHECK-NEXT: jl .L{{.*}}
; CHECK: br %r14
entry:
  %res = and i64 %b, -2
  %cmp = icmp slt i64 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i64 %b, i64 *%dest
  br label %exit

exit:
  ret i64 %res
}

; Test a case where the register we're testing is set by a non-CC-clobbering
; instruction.
define i32 @f21(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f21:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: #APP
; CHECK-NEXT: blah %r2
; CHECK-NEXT: #NO_APP
; CHECK-NEXT: cije %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %add = add i32 %a, 1000000
  %res = call i32 asm "blah $0", "=r,0" (i32 %add)
  %cmp = icmp eq i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; ...and again with a CC-clobbering instruction.
define i32 @f22(i32 %a, i32 %b, i32 *%dest) {
; CHECK-LABEL: f22:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: #APP
; CHECK-NEXT: blah %r2
; CHECK-NEXT: #NO_APP
; CHECK-NEXT: cije %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %add = add i32 %a, 1000000
  %res = call i32 asm "blah $0", "=r,0,~{cc}" (i32 %add)
  %cmp = icmp eq i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest
  br label %exit

exit:
  ret i32 %res
}

; Check that stores do not interfere.
define i32 @f23(i32 %a, i32 %b, i32 *%dest1, i32 *%dest2) {
; CHECK-LABEL: f23:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: st %r2, 0(%r4)
; CHECK-NEXT: jne .L{{.*}}
; CHECK: br %r14
entry:
  %res = add i32 %a, 1000000
  store i32 %res, i32 *%dest1
  %cmp = icmp ne i32 %res, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %b, i32 *%dest2
  br label %exit

exit:
  ret i32 %res
}

; Check that calls do interfere.
define void @f24(i32 *%ptr) {
; CHECK-LABEL: f24:
; CHECK: afi [[REG:%r[0-9]+]], 1000000
; CHECK-NEXT: brasl %r14, foo@PLT
; CHECK-NEXT: cijlh [[REG]], 0, .L{{.*}}
; CHECK: br %r14
entry:
  %val = load i32 *%ptr
  %xor = xor i32 %val, 1
  %add = add i32 %xor, 1000000
  call void @foo()
  %cmp = icmp ne i32 %add, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %add, i32 *%ptr
  br label %exit

exit:
  ret void
}

; Check that inline asms don't interfere if they don't clobber CC.
define void @f25(i32 %a, i32 *%ptr) {
; CHECK-LABEL: f25:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: #APP
; CHECK-NEXT: blah
; CHECK-NEXT: #NO_APP
; CHECK-NEXT: jne .L{{.*}}
; CHECK: br %r14
entry:
  %add = add i32 %a, 1000000
  call void asm sideeffect "blah", "r"(i32 %add)
  %cmp = icmp ne i32 %add, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %add, i32 *%ptr
  br label %exit

exit:
  ret void
}

; ...but do interfere if they do clobber CC.
define void @f26(i32 %a, i32 *%ptr) {
; CHECK-LABEL: f26:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: #APP
; CHECK-NEXT: blah
; CHECK-NEXT: #NO_APP
; CHECK-NEXT: cijlh %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %add = add i32 %a, 1000000
  call void asm sideeffect "blah", "r,~{cc}"(i32 %add)
  %cmp = icmp ne i32 %add, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %add, i32 *%ptr
  br label %exit

exit:
  ret void
}

; Test a case where CC is set based on a different register from the
; compare input.
define i32 @f27(i32 %a, i32 %b, i32 *%dest1, i32 *%dest2) {
; CHECK-LABEL: f27:
; CHECK: afi %r2, 1000000
; CHECK-NEXT: sr %r3, %r2
; CHECK-NEXT: st %r3, 0(%r4)
; CHECK-NEXT: cije %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %add = add i32 %a, 1000000
  %sub = sub i32 %b, %add
  store i32 %sub, i32 *%dest1
  %cmp = icmp eq i32 %add, 0
  br i1 %cmp, label %exit, label %store

store:
  store i32 %sub, i32 *%dest2
  br label %exit

exit:
  ret i32 %add
}

; Make sure that we don't confuse a base register for a destination.
define void @f28(i64 %a, i64 *%dest) {
; CHECK-LABEL: f28:
; CHECK: xi 0(%r2), 15
; CHECK: cgije %r2, 0, .L{{.*}}
; CHECK: br %r14
entry:
  %ptr = inttoptr i64 %a to i8 *
  %val = load i8 *%ptr
  %xor = xor i8 %val, 15
  store i8 %xor, i8 *%ptr
  %cmp = icmp eq i64 %a, 0
  br i1 %cmp, label %exit, label %store

store:
  store i64 %a, i64 *%dest
  br label %exit

exit:
  ret void
}
