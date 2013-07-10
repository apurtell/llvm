; RUN: not llc < %s 2> %t
; RUN: FileCheck --check-prefix=CHECK-ERRORS < %t %s
; XFAIL: hexagon
; REQUIRES: shell

@0 = global i8 extractvalue ([1 x i8] select (i1 ptrtoint (i32* @1 to i1), [1 x i8] [ i8 1 ], [1 x i8] [ i8 2 ]), 0)
@1 = external global i32

; CHECK-ERRORS: Unsupported expression in static initializer: extractvalue

