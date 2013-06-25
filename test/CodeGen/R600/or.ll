;RUN: llc < %s -march=r600 -mcpu=redwood | FileCheck --check-prefix=EG-CHECK %s
;RUN: llc < %s -march=r600 -mcpu=verde | FileCheck --check-prefix=SI-CHECK %s

; EG-CHECK: @or_v2i32
; EG-CHECK: OR_INT {{\*? *}}T{{[0-9]+\.[XYZW], T[0-9]+\.[XYZW], T[0-9]+\.[XYZW]}}
; EG-CHECK: OR_INT {{\*? *}}T{{[0-9]+\.[XYZW], T[0-9]+\.[XYZW], T[0-9]+\.[XYZW]}}

;SI-CHECK: @or_v2i32
;SI-CHECK: V_OR_B32_e32 VGPR{{[0-9]+, VGPR[0-9]+, VGPR[0-9]+}}
;SI-CHECK: V_OR_B32_e32 VGPR{{[0-9]+, VGPR[0-9]+, VGPR[0-9]+}}

define void @or_v2i32(<2 x i32> addrspace(1)* %out, <2 x i32> addrspace(1)* %in) {
  %b_ptr = getelementptr <2 x i32> addrspace(1)* %in, i32 1
  %a = load <2 x i32> addrspace(1) * %in
  %b = load <2 x i32> addrspace(1) * %b_ptr
  %result = or <2 x i32> %a, %b
  store <2 x i32> %result, <2 x i32> addrspace(1)* %out
  ret void
}

; EG-CHECK: @or_v4i32
; EG-CHECK: OR_INT {{\*? *}}T{{[0-9]+\.[XYZW], T[0-9]+\.[XYZW], T[0-9]+\.[XYZW]}}
; EG-CHECK: OR_INT {{\*? *}}T{{[0-9]+\.[XYZW], T[0-9]+\.[XYZW], T[0-9]+\.[XYZW]}}
; EG-CHECK: OR_INT {{\*? *}}T{{[0-9]+\.[XYZW], T[0-9]+\.[XYZW], T[0-9]+\.[XYZW]}}
; EG-CHECK: OR_INT {{\*? *}}T{{[0-9]+\.[XYZW], T[0-9]+\.[XYZW], T[0-9]+\.[XYZW]}}

;SI-CHECK: @or_v4i32
;SI-CHECK: V_OR_B32_e32 VGPR{{[0-9]+, VGPR[0-9]+, VGPR[0-9]+}}
;SI-CHECK: V_OR_B32_e32 VGPR{{[0-9]+, VGPR[0-9]+, VGPR[0-9]+}}
;SI-CHECK: V_OR_B32_e32 VGPR{{[0-9]+, VGPR[0-9]+, VGPR[0-9]+}}
;SI-CHECK: V_OR_B32_e32 VGPR{{[0-9]+, VGPR[0-9]+, VGPR[0-9]+}}

define void @or_v4i32(<4 x i32> addrspace(1)* %out, <4 x i32> addrspace(1)* %in) {
  %b_ptr = getelementptr <4 x i32> addrspace(1)* %in, i32 1
  %a = load <4 x i32> addrspace(1) * %in
  %b = load <4 x i32> addrspace(1) * %b_ptr
  %result = or <4 x i32> %a, %b
  store <4 x i32> %result, <4 x i32> addrspace(1)* %out
  ret void
}
