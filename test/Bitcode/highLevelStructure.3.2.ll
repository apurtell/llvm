; RUN:  llvm-dis < %s.bc| FileCheck %s

; highLevelStructure.3.2.ll.bc was generated by passing this file to llvm-as-3.2.
; The test checks that LLVM does not misread binary float instructions of
; older bitcode files.

; Data Layout Test
; CHECK: target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-f80:32-n8:16:32-S32"
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a0:0:64-f80:32:32-n8:16:32-S32"

; Module-Level Inline Assembly Test
; CHECK: module asm "some assembly"
module asm "some assembly"

; Named Types Test
; CHECK: %mytype = type { %mytype*, i32 }
%mytype = type { %mytype*, i32 }

; Aliases Test
; CHECK: @glob1 = global i32 1
@glob1 = global i32 1
; CHECK: @aliased1 = alias i32* @glob1
@aliased1 = alias i32* @glob1
; CHECK-NEXT: @aliased2 = internal alias i32* @glob1
@aliased2 = internal alias i32* @glob1
; CHECK-NEXT: @aliased3 = alias i32* @glob1
@aliased3 = external alias i32* @glob1
; CHECK-NEXT: @aliased4 = weak alias i32* @glob1
@aliased4 = weak alias i32* @glob1
; CHECK-NEXT: @aliased5 = weak_odr alias i32* @glob1
@aliased5 = weak_odr alias i32* @glob1

;Parameter Attribute Test
; CHECK: declare void @ParamAttr1(i8 zeroext)
declare void @ParamAttr1(i8 zeroext)
; CHECK: declare void @ParamAttr2(i8* nest)
declare void @ParamAttr2(i8* nest)
; CHECK: declare void @ParamAttr3(i8* sret)
declare void @ParamAttr3(i8* sret)
; CHECK: declare void @ParamAttr4(i8 signext)
declare void @ParamAttr4(i8 signext)
; CHECK: declare void @ParamAttr5(i8* inreg)
declare void @ParamAttr5(i8* inreg)
; CHECK: declare void @ParamAttr6(i8* byval)
declare void @ParamAttr6(i8* byval)
; CHECK: declare void @ParamAttr7(i8* noalias)
declare void @ParamAttr7(i8* noalias)
; CHECK: declare void @ParamAttr8(i8* nocapture)
declare void @ParamAttr8(i8* nocapture)
; CHECK: declare void @ParamAttr9{{[(i8* nest noalias nocapture) | (i8* noalias nocapture nest)]}}
declare void @ParamAttr9(i8* nest noalias nocapture)
; CHECK: declare void @ParamAttr10{{[(i8* sret noalias nocapture) | (i8* noalias nocapture sret)]}}
declare void @ParamAttr10(i8* sret noalias nocapture)
;CHECK: declare void @ParamAttr11{{[(i8* byval noalias nocapture) | (i8* noalias nocapture byval)]}}
declare void @ParamAttr11(i8* byval noalias nocapture)
;CHECK: declare void @ParamAttr12{{[(i8* inreg noalias nocapture) | (i8* noalias nocapture inreg)]}}
declare void @ParamAttr12(i8* inreg noalias nocapture)


; NamedTypesTest
define void @NamedTypes() {
entry:
; CHECK: %res = alloca %mytype
  %res = alloca %mytype
  ret void
}

; Garbage Collector Name Test
; CHECK: define void @gcTest() gc "gc"
define void @gcTest() gc "gc" {
entry:
  ret void
}

; Named metadata Test
; CHECK: !name = !{!0, !1, !2}
!name = !{!0, !1, !2}
; CHECK: !0 = metadata !{metadata !"zero"}
!0 = metadata !{metadata !"zero"}
; CHECK: !1 = metadata !{metadata !"one"}
!1 = metadata !{metadata !"one"}
; CHECK: !2 = metadata !{metadata !"two"}
!2 = metadata !{metadata !"two"}



