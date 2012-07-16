; Test basic address sanitizer instrumentation.
;
; RUN: opt < %s -asan -S | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @test_load(i32* %a) address_safety {
; CHECK: @test_load
; CHECK-NOT: load
; CHECK:   %[[LOAD_ADDR:[^ ]*]] = ptrtoint i32* %a to i64
; CHECK:   lshr i64 %[[LOAD_ADDR]], 3
; CHECK:   or i64
; CHECK:   %[[LOAD_SHADOW_PTR:[^ ]*]] = inttoptr
; CHECK:   %[[LOAD_SHADOW:[^ ]*]] = load i8* %[[LOAD_SHADOW_PTR]]
; CHECK:   icmp ne i8
; CHECK:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
;
; The actual load comes next because ASan adds the last instrumentation block
; to the end of the function.
; CHECK:   %tmp1 = load i32* %a
; CHECK:   ret i32 %tmp1
;
; First instrumentation block refines the shadow test.
; CHECK:   and i64 %[[LOAD_ADDR]], 7
; CHECK:   add i64 %{{.*}}, 3
; CHECK:   trunc i64 %{{.*}} to i8
; CHECK:   icmp sge i8 %{{.*}}, %[[LOAD_SHADOW]]
; CHECK:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
;
; Final instrumentation block reports the error.
; CHECK:   call void @__asan_report_load4(i64 %[[LOAD_ADDR]]) noreturn
; CHECK:   unreachable

entry:
  %tmp1 = load i32* %a
  ret i32 %tmp1
}

define void @test_store(i32* %a) address_safety {
; CHECK: @test_store
; CHECK-NOT: store
; CHECK:   %[[STORE_ADDR:[^ ]*]] = ptrtoint i32* %a to i64
; CHECK:   lshr i64 %[[STORE_ADDR]], 3
; CHECK:   or i64
; CHECK:   %[[STORE_SHADOW_PTR:[^ ]*]] = inttoptr
; CHECK:   %[[STORE_SHADOW:[^ ]*]] = load i8* %[[STORE_SHADOW_PTR]]
; CHECK:   icmp ne i8
; CHECK:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
;
; The actual store comes next because ASan adds the last instrumentation block
; to the end of the function.
; CHECK:   store i32 42, i32* %a
; CHECK:   ret void
;
; First instrumentation block refines the shadow test.
; CHECK:   and i64 %[[STORE_ADDR]], 7
; CHECK:   add i64 %{{.*}}, 3
; CHECK:   trunc i64 %{{.*}} to i8
; CHECK:   icmp sge i8 %{{.*}}, %[[STORE_SHADOW]]
; CHECK:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
;
; Final instrumentation block reports the error.
; CHECK:   call void @__asan_report_store4(i64 %[[STORE_ADDR]]) noreturn
; CHECK:   unreachable

entry:
  store i32 42, i32* %a
  ret void
}
