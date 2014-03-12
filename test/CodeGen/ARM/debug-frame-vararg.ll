; RUN: llc -mtriple arm-unknown-linux-gnueabi -filetype asm -o - %s | FileCheck %s --check-prefix=CHECK-FP
; RUN: llc -mtriple arm-unknown-linux-gnueabi -filetype asm -o - %s -disable-fp-elim | FileCheck %s --check-prefix=CHECK-FP-ELIM
; RUN: llc -mtriple thumb-unknown-linux-gnueabi -filetype asm -o - %s | FileCheck %s --check-prefix=CHECK-THUMB-FP
; RUN: llc -mtriple thumb-unknown-linux-gnueabi -filetype asm -o - %s -disable-fp-elim | FileCheck %s --check-prefix=CHECK-THUMB-FP-ELIM

; Tests that the initial space allocated to the varargs on the stack is
; taken into account in the the .cfi_ directives.

; Generated from the C program:
; #include <stdarg.h>
;
; extern int foo(int);
;
; int sum(int count, ...) {
;  va_list vl;
;  va_start(vl, count);
;  int sum = 0;
;  for (int i = 0; i < count; i++) {
;   sum += foo(va_arg(vl, int));
;  }
;  va_end(vl);
; }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9, !10}
!llvm.ident = !{!11}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.5 ", i1 false, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [/tmp/var.c] [DW_LANG_C99]
!1 = metadata !{metadata !"var.c", metadata !"/tmp"}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"sum", metadata !"sum", metadata !"", i32 5, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (i32, ...)* @sum, null, null, metadata !2, i32 5} ; [ DW_TAG_subprogram ] [line 5] [def] [sum]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/tmp/var.c]
!6 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{metadata !8, metadata !8}
!8 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!9 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!10 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!11 = metadata !{metadata !"clang version 3.5 "}
!12 = metadata !{i32 786689, metadata !4, metadata !"count", metadata !5, i32 16777221, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [count] [line 5]
!13 = metadata !{i32 5, i32 0, metadata !4, null}
!14 = metadata !{i32 786688, metadata !4, metadata !"vl", metadata !5, i32 6, metadata !15, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [vl] [line 6]
!15 = metadata !{i32 786454, metadata !16, null, metadata !"va_list", i32 30, i64 0, i64 0, i64 0, i32 0, metadata !17} ; [ DW_TAG_typedef ] [va_list] [line 30, size 0, align 0, offset 0] [from __builtin_va_list]
!16 = metadata !{metadata !"/linux-x86_64-high/gcc_4.7.2/dbg/llvm/bin/../lib/clang/3.5/include/stdarg.h", metadata !"/tmp"}
!17 = metadata !{i32 786454, metadata !1, null, metadata !"__builtin_va_list", i32 6, i64 0, i64 0, i64 0, i32 0, metadata !18} ; [ DW_TAG_typedef ] [__builtin_va_list] [line 6, size 0, align 0, offset 0] [from __va_list]
!18 = metadata !{i32 786451, metadata !1, null, metadata !"__va_list", i32 6, i64 32, i64 32, i32 0, i32 0, null, metadata !19, i32 0, null, null, null} ; [ DW_TAG_structure_type ] [__va_list] [line 6, size 32, align 32, offset 0] [def] [from ]
!19 = metadata !{metadata !20}
!20 = metadata !{i32 786445, metadata !1, metadata !18, metadata !"__ap", i32 6, i64 32, i64 32, i64 0, i32 0, metadata !21} ; [ DW_TAG_member ] [__ap] [line 6, size 32, align 32, offset 0] [from ]
!21 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, null} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from ]
!22 = metadata !{i32 6, i32 0, metadata !4, null}
!23 = metadata !{i32 7, i32 0, metadata !4, null}
!24 = metadata !{i32 786688, metadata !4, metadata !"sum", metadata !5, i32 8, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [sum] [line 8]
!25 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!26 = metadata !{i32 786688, metadata !27, metadata !"i", metadata !5, i32 9, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 9]
!27 = metadata !{i32 786443, metadata !1, metadata !4, i32 9, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/tmp/var.c]
!28 = metadata !{i32 9, i32 0, metadata !27, null}
!29 = metadata !{i32 10, i32 0, metadata !30, null}
!30 = metadata !{i32 786443, metadata !1, metadata !27, i32 9, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/tmp/var.c]
!31 = metadata !{i32 11, i32 0, metadata !30, null}
!32 = metadata !{i32 12, i32 0, metadata !4, null}
!33 = metadata !{i32 13, i32 0, metadata !4, null}

; CHECK-FP-LABEL: sum
; CHECK-FP: .cfi_startproc
; CHECK-FP: sub    sp, sp, #16
; CHECK-FP: .cfi_def_cfa_offset 16
; CHECK-FP: push   {r4, lr}
; CHECK-FP: .cfi_def_cfa_offset 24
; CHECK-FP: .cfi_offset 14, -20
; CHECK-FP: .cfi_offset 4, -24
; CHECK-FP: sub    sp, sp, #8
; CHECK-FP: .cfi_def_cfa_offset 32

; CHECK-FP-ELIM-LABEL: sum
; CHECK-FP-ELIM: .cfi_startproc
; CHECK-FP-ELIM: sub    sp, sp, #16
; CHECK-FP-ELIM: .cfi_def_cfa_offset 16
; CHECK-FP-ELIM: push   {r4, r11, lr}
; CHECK-FP-ELIM: .cfi_def_cfa_offset 28
; CHECK-FP-ELIM: .cfi_offset 14, -20
; CHECK-FP-ELIM: .cfi_offset 11, -24
; CHECK-FP-ELIM: .cfi_offset 4, -28
; CHECK-FP-ELIM: add    r11, sp, #4
; CHECK-FP-ELIM: .cfi_def_cfa 11, 24

; CHECK-THUMB-FP-LABEL: sum
; CHECK-THUMB-FP: .cfi_startproc
; CHECK-THUMB-FP: sub    sp, #16
; CHECK-THUMB-FP: .cfi_def_cfa_offset 16
; CHECK-THUMB-FP: push   {r4, r5, r7, lr}
; CHECK-THUMB-FP: .cfi_def_cfa_offset 32
; CHECK-THUMB-FP: .cfi_offset 14, -20
; CHECK-THUMB-FP: .cfi_offset 7, -24
; CHECK-THUMB-FP: .cfi_offset 5, -28
; CHECK-THUMB-FP: .cfi_offset 4, -32
; CHECK-THUMB-FP: sub    sp, #8
; CHECK-THUMB-FP: .cfi_def_cfa_offset 40

; CHECK-THUMB-FP-ELIM-LABEL: sum
; CHECK-THUMB-FP-ELIM: .cfi_startproc
; CHECK-THUMB-FP-ELIM: sub    sp, #16
; CHECK-THUMB-FP-ELIM: .cfi_def_cfa_offset 16
; CHECK-THUMB-FP-ELIM: push   {r4, r5, r7, lr}
; CHECK-THUMB-FP-ELIM: .cfi_def_cfa_offset 32
; CHECK-THUMB-FP-ELIM: .cfi_offset 14, -20
; CHECK-THUMB-FP-ELIM: .cfi_offset 7, -24
; CHECK-THUMB-FP-ELIM: .cfi_offset 5, -28
; CHECK-THUMB-FP-ELIM: .cfi_offset 4, -32
; CHECK-THUMB-FP-ELIM: add    r7, sp, #8
; CHECK-THUMB-FP-ELIM: .cfi_def_cfa 7, 24

define i32 @sum(i32 %count, ...) {
entry:
  %vl = alloca i8*, align 4
  %vl1 = bitcast i8** %vl to i8*
  call void @llvm.va_start(i8* %vl1)
  %cmp4 = icmp sgt i32 %count, 0
  br i1 %cmp4, label %for.body, label %for.end

for.body:                                         ; preds = %entry, %for.body
  %i.05 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %ap.cur = load i8** %vl, align 4
  %ap.next = getelementptr i8* %ap.cur, i32 4
  store i8* %ap.next, i8** %vl, align 4
  %0 = bitcast i8* %ap.cur to i32*
  %1 = load i32* %0, align 4
  %call = call i32 @foo(i32 %1) #1
  %inc = add nsw i32 %i.05, 1
  %exitcond = icmp eq i32 %inc, %count
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %for.body, %entry
  call void @llvm.va_end(i8* %vl1)
  ret i32 undef
}

declare void @llvm.va_start(i8*) nounwind

declare i32 @foo(i32)

declare void @llvm.va_end(i8*) nounwind
