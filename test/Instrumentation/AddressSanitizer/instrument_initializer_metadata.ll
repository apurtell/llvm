; RUN: opt < %s -asan -asan-module -asan-initialization-order -S | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
@xxx = internal global i32 0, align 4  ; With dynamic initializer.
@XXX = global i32 0, align 4           ; With dynamic initializer.
@yyy = internal global i32 0, align 4  ; W/o dynamic initializer.
@YYY = global i32 0, align 4           ; W/o dynamic initializer.
; Clang will emit the following metadata identifying @xxx as dynamically
; initialized.
!0 = metadata !{i32* @xxx}
!1 = metadata !{i32* @XXX}
!llvm.asan.dynamically_initialized_globals = !{!0, !1}

define i32 @initializer() uwtable {
entry:
  ret i32 42
}

define internal void @__cxx_global_var_init() section ".text.startup" {
entry:
  %call = call i32 @initializer()
  store i32 %call, i32* @xxx, align 4
  ret void
}

define internal void @_GLOBAL__I_a() sanitize_address section ".text.startup" {
entry:
  call void @__cxx_global_var_init()
  ret void
}

; Clang indicated that @xxx was dynamically initailized.
; __asan_{before,after}_dynamic_init should be called from _GLOBAL__I_a

; CHECK: define internal void @_GLOBAL__I_a
; CHECK-NOT: ret
; CHECK: call void @__asan_before_dynamic_init
; CHECK: call void @__cxx_global_var_init
; CHECK: call void @__asan_after_dynamic_init
; CHECK: ret

; Check that xxx is instrumented.
define void @touch_xxx() sanitize_address {
  store i32 0, i32 *@xxx, align 4
  ret void
; CHECK: define void @touch_xxx
; CHECK: call void @__asan_report_store4
; CHECK: ret void
}

; Check that XXX is instrumented.
define void @touch_XXX() sanitize_address {
  store i32 0, i32 *@XXX, align 4
  ret void
; CHECK: define void @touch_XXX
; CHECK: call void @__asan_report_store4
; CHECK: ret void
}


; Check that yyy is NOT instrumented (as it does not have dynamic initializer).
define void @touch_yyy() sanitize_address {
  store i32 0, i32 *@yyy, align 4
  ret void
; CHECK: define void @touch_yyy
; CHECK-NOT: call void @__asan_report_store4
; CHECK: ret void
}

; Check that YYY is NOT instrumented (as it does not have dynamic initializer).
define void @touch_YYY() sanitize_address {
  store i32 0, i32 *@YYY, align 4
  ret void
; CHECK: define void @touch_YYY
; CHECK-NOT: call void @__asan_report_store4
; CHECK: ret void
}
