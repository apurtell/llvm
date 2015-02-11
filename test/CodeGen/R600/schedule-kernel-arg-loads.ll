; RUN: llc -march=amdgcn -mcpu=SI -verify-machineinstrs < %s | FileCheck -check-prefix=FUNC -check-prefix=SI %s
; RUN: llc -march=amdgcn -mcpu=tonga -verify-machineinstrs < %s | FileCheck -check-prefix=FUNC -check-prefix=VI %s

; FUNC-LABEL: {{^}}cluster_arg_loads:
; SI: s_load_dwordx2 s{{\[[0-9]+:[0-9]+\]}}, s{{\[[0-9]+:[0-9]+\]}}, 0x9
; SI-NEXT: s_load_dwordx2 s{{\[[0-9]+:[0-9]+\]}}, s{{\[[0-9]+:[0-9]+\]}}, 0xb
; SI-NEXT: s_load_dword s{{[0-9]+}}, s{{\[[0-9]+:[0-9]+\]}}, 0xd
; SI-NEXT: s_load_dword s{{[0-9]+}}, s{{\[[0-9]+:[0-9]+\]}}, 0xe
; VI: s_load_dwordx2 s{{\[[0-9]+:[0-9]+\]}}, s{{\[[0-9]+:[0-9]+\]}}, 0x24
; VI-NEXT: s_nop 0
; VI-NEXT: s_load_dwordx2 s{{\[[0-9]+:[0-9]+\]}}, s{{\[[0-9]+:[0-9]+\]}}, 0x2c
; VI-NEXT: s_nop 0
; VI-NEXT: s_load_dword s{{[0-9]+}}, s{{\[[0-9]+:[0-9]+\]}}, 0x34
; VI-NEXT: s_nop 0
; VI-NEXT: s_load_dword s{{[0-9]+}}, s{{\[[0-9]+:[0-9]+\]}}, 0x38
define void @cluster_arg_loads(i32 addrspace(1)* %out0, i32 addrspace(1)* %out1, i32 %x, i32 %y) nounwind {
  store i32 %x, i32 addrspace(1)* %out0, align 4
  store i32 %y, i32 addrspace(1)* %out1, align 4
  ret void
}
