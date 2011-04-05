; RUN: llc -march=arm < %s | FileCheck %s

; 171 = 0x000000ab
define i64 @f1(i64 %a) {
; CHECK: f1
; CHECK: subs r0, r0, #171
; CHECK: sbc r1, r1, #0
    %tmp = sub i64 %a, 171
    ret i64 %tmp
}

; 66846720 = 0x03fc0000
define i64 @f2(i64 %a) {
; CHECK: f2
; CHECK: subs r0, r0, #255, #14
; CHECK: sbc r1, r1, #0
    %tmp = sub i64 %a, 66846720
    ret i64 %tmp
}

; 734439407618 = 0x000000ab00000002
define i64 @f3(i64 %a) {
; CHECK: f3
; CHECK: subs r0, r0, #2
; CHECK: sbc r1, r1, #171
   %tmp = sub i64 %a, 734439407618
   ret i64 %tmp
}

