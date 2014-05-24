; RUN: llc -mtriple=arm64-none-linux-gnu -o - %s | FileCheck %s --check-prefix=CHECK-ARM64
; RUN: llc -mtriple=arm64-none-linux-gnu -code-model=large -o - %s | FileCheck --check-prefix=CHECK-LARGE %s

declare extern_weak i32 @var()

define i32()* @foo() {
; The usual ADRP/ADD pair can't be used for a weak reference because it must
; evaluate to 0 if the symbol is undefined. We use a litpool entry.
  ret i32()* @var


; CHECK-ARM64: adrp x[[ADDRHI:[0-9]+]], :got:var
; CHECK-ARM64: ldr x0, [x[[ADDRHI]], :got_lo12:var]

  ; In the large model, the usual relocations are absolute and can
  ; materialise 0.
; CHECK-LARGE: movz x0, #:abs_g3:var
; CHECK-LARGE: movk x0, #:abs_g2_nc:var
; CHECK-LARGE: movk x0, #:abs_g1_nc:var
; CHECK-LARGE: movk x0, #:abs_g0_nc:var
}


@arr_var = extern_weak global [10 x i32]

define i32* @bar() {
  %addr = getelementptr [10 x i32]* @arr_var, i32 0, i32 5


; CHECK-ARM64: adrp x[[ADDRHI:[0-9]+]], :got:arr_var
; CHECK-ARM64: ldr [[BASE:x[0-9]+]], [x[[ADDRHI]], :got_lo12:arr_var]
; CHECK-ARM64: add x0, [[BASE]], #20

  ret i32* %addr

  ; In the large model, the usual relocations are absolute and can
  ; materialise 0.
; CHECK-LARGE: movz [[ADDR:x[0-9]+]], #:abs_g3:arr_var
; CHECK-LARGE: movk [[ADDR]], #:abs_g2_nc:arr_var
; CHECK-LARGE: movk [[ADDR]], #:abs_g1_nc:arr_var
; CHECK-LARGE: movk [[ADDR]], #:abs_g0_nc:arr_var
}

@defined_weak_var = internal unnamed_addr global i32 0

define i32* @wibble() {
  ret i32* @defined_weak_var

; CHECK-ARM64: adrp [[BASE:x[0-9]+]], defined_weak_var
; CHECK-ARM64: add x0, [[BASE]], :lo12:defined_weak_var

; CHECK-LARGE: movz x0, #:abs_g3:defined_weak_var
; CHECK-LARGE: movk x0, #:abs_g2_nc:defined_weak_var
; CHECK-LARGE: movk x0, #:abs_g1_nc:defined_weak_var
; CHECK-LARGE: movk x0, #:abs_g0_nc:defined_weak_var
}
