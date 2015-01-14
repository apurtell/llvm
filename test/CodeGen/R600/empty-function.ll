; RUN: llc -march=amdgcn -mcpu=SI -verify-machineinstrs < %s | FileCheck -check-prefix=SI %s

; Make sure we don't assert on empty functions

; SI-LABEL: {{^}}empty_function_ret:
; SI: .text
; SI: s_endpgm
; SI: codeLenInByte = 4
define void @empty_function_ret() #0 {
  ret void
}

; SI-LABEL: {{^}}empty_function_unreachable:
; SI: .text
; SI: codeLenInByte = 0
define void @empty_function_unreachable() #0 {
  unreachable
}

attributes #0 = { nounwind }
