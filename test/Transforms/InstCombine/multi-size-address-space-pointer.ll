; RUN: opt -S -instcombine %s -o - | FileCheck %s
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:8:8:8-p3:16:16:16-p4:16:16:16-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32"


define i32 @test_as0(i32 addrspace(0)* %a) {
; CHECK-LABEL: @test_as0(
; CHECK: %arrayidx = getelementptr i32, i32* %a, i32 1
  %arrayidx = getelementptr i32, i32 addrspace(0)* %a, i64 1
  %y = load i32, i32 addrspace(0)* %arrayidx, align 4
  ret i32 %y
}

define i32 @test_as1(i32 addrspace(1)* %a) {
; CHECK-LABEL: @test_as1(
; CHECK: %arrayidx = getelementptr i32, i32 addrspace(1)* %a, i64 1
  %arrayidx = getelementptr i32, i32 addrspace(1)* %a, i32 1
  %y = load i32, i32 addrspace(1)* %arrayidx, align 4
  ret i32 %y
}

define i32 @test_as2(i32 addrspace(2)* %a) {
; CHECK-LABEL: @test_as2(
; CHECK: %arrayidx = getelementptr i32, i32 addrspace(2)* %a, i8 1
  %arrayidx = getelementptr i32, i32 addrspace(2)* %a, i32 1
  %y = load i32, i32 addrspace(2)* %arrayidx, align 4
  ret i32 %y
}

define i32 @test_as3(i32 addrspace(3)* %a) {
; CHECK-LABEL: @test_as3(
; CHECK: %arrayidx = getelementptr i32, i32 addrspace(3)* %a, i16 1
  %arrayidx = getelementptr i32, i32 addrspace(3)* %a, i32 1
  %y = load i32, i32 addrspace(3)* %arrayidx, align 4
  ret i32 %y
}

define i32 @test_combine_ptrtoint(i32 addrspace(2)* %a) {
; CHECK-LABEL: @test_combine_ptrtoint(
; CHECK-NEXT: %y = load i32, i32 addrspace(2)* %a
; CHECK-NEXT: ret i32 %y
  %cast = ptrtoint i32 addrspace(2)* %a to i8
  %castback = inttoptr i8 %cast to i32 addrspace(2)*
  %y = load i32, i32 addrspace(2)* %castback, align 4
  ret i32 %y
}

define i8 @test_combine_inttoptr(i8 %a) {
; CHECK-LABEL: @test_combine_inttoptr(
; CHECK-NEXT: ret i8 %a
  %cast = inttoptr i8 %a to i32 addrspace(2)*
  %castback = ptrtoint i32 addrspace(2)* %cast to i8
  ret i8 %castback
}

define i32 @test_combine_vector_ptrtoint(<2 x i32 addrspace(2)*> %a) {
; CHECK-LABEL: @test_combine_vector_ptrtoint(
; CHECK-NEXT: %p = extractelement <2 x i32 addrspace(2)*> %a, i32 0
; CHECK-NEXT: %y = load i32, i32 addrspace(2)* %p, align 4
; CHECK-NEXT: ret i32 %y
  %cast = ptrtoint <2 x i32 addrspace(2)*> %a to <2 x i8>
  %castback = inttoptr <2 x i8> %cast to <2 x i32 addrspace(2)*>
  %p = extractelement <2 x i32 addrspace(2)*> %castback, i32 0
  %y = load i32, i32 addrspace(2)* %p, align 4
  ret i32 %y
}

define <2 x i8> @test_combine_vector_inttoptr(<2 x i8> %a) {
; CHECK-LABEL: @test_combine_vector_inttoptr(
; CHECK-NEXT: ret <2 x i8> %a
  %cast = inttoptr <2 x i8> %a to <2 x i32 addrspace(2)*>
  %castback = ptrtoint <2 x i32 addrspace(2)*> %cast to <2 x i8>
  ret <2 x i8> %castback
}

; Check that the GEP index is changed to the address space integer type (i64 -> i8)
define i32 addrspace(2)* @shrink_gep_constant_index_64_as2(i32 addrspace(2)* %p) {
; CHECK-LABEL: @shrink_gep_constant_index_64_as2(
; CHECK-NEXT: getelementptr i32, i32 addrspace(2)* %p, i8 1
  %ret = getelementptr i32, i32 addrspace(2)* %p, i64 1
  ret i32 addrspace(2)* %ret
}

define i32 addrspace(2)* @shrink_gep_constant_index_32_as2(i32 addrspace(2)* %p) {
; CHECK-LABEL: @shrink_gep_constant_index_32_as2(
; CHECK-NEXT: getelementptr i32, i32 addrspace(2)* %p, i8 1
  %ret = getelementptr i32, i32 addrspace(2)* %p, i32 1
  ret i32 addrspace(2)* %ret
}

define i32 addrspace(3)* @shrink_gep_constant_index_64_as3(i32 addrspace(3)* %p) {
; CHECK-LABEL: @shrink_gep_constant_index_64_as3(
; CHECK-NEXT: getelementptr i32, i32 addrspace(3)* %p, i16 1
  %ret = getelementptr i32, i32 addrspace(3)* %p, i64 1
  ret i32 addrspace(3)* %ret
}

define i32 addrspace(2)* @shrink_gep_variable_index_64_as2(i32 addrspace(2)* %p, i64 %idx) {
; CHECK-LABEL: @shrink_gep_variable_index_64_as2(
; CHECK-NEXT: %1 = trunc i64 %idx to i8
; CHECK-NEXT: getelementptr i32, i32 addrspace(2)* %p, i8 %1
  %ret = getelementptr i32, i32 addrspace(2)* %p, i64 %idx
  ret i32 addrspace(2)* %ret
}

define i32 addrspace(1)* @grow_gep_variable_index_8_as1(i32 addrspace(1)* %p, i8 %idx) {
; CHECK-LABEL: @grow_gep_variable_index_8_as1(
; CHECK-NEXT: %1 = sext i8 %idx to i64
; CHECK-NEXT: getelementptr i32, i32 addrspace(1)* %p, i64 %1
  %ret = getelementptr i32, i32 addrspace(1)* %p, i8 %idx
  ret i32 addrspace(1)* %ret
}

