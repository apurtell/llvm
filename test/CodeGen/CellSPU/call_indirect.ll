; RUN: llvm-as -o - %s | llc -march=cellspu > %t1.s
; RUN: grep bisl    %t1.s | count 6 &&
; RUN: grep ila     %t1.s | count 1 &&
; RUN: grep rotqbyi %t1.s | count 4 &&
; RUN: grep lqa     %t1.s | count 4 &&
; RUN: grep lqd     %t1.s | count 6 &&
; RUN: grep dispatch_tab %t1.s | count 10
; ModuleID = 'call_indirect.bc'
target datalayout = "E-p:32:32:128-f64:64:128-f32:32:128-i64:32:128-i32:32:128-i16:16:128-i8:8:128-i1:8:128-a0:0:128-v128:128:128"
target triple = "spu-unknown-elf"

@dispatch_tab = global [6 x void (i32, float)*] zeroinitializer, align 16

define void @dispatcher(i32 %i_arg, float %f_arg) {
entry:
	%tmp2 = load void (i32, float)** getelementptr ([6 x void (i32, float)*]* @dispatch_tab, i32 0, i32 0), align 16
	tail call void %tmp2( i32 %i_arg, float %f_arg )
	%tmp2.1 = load void (i32, float)** getelementptr ([6 x void (i32, float)*]* @dispatch_tab, i32 0, i32 1), align 4
	tail call void %tmp2.1( i32 %i_arg, float %f_arg )
	%tmp2.2 = load void (i32, float)** getelementptr ([6 x void (i32, float)*]* @dispatch_tab, i32 0, i32 2), align 4
	tail call void %tmp2.2( i32 %i_arg, float %f_arg )
	%tmp2.3 = load void (i32, float)** getelementptr ([6 x void (i32, float)*]* @dispatch_tab, i32 0, i32 3), align 4
	tail call void %tmp2.3( i32 %i_arg, float %f_arg )
	%tmp2.4 = load void (i32, float)** getelementptr ([6 x void (i32, float)*]* @dispatch_tab, i32 0, i32 4), align 4
	tail call void %tmp2.4( i32 %i_arg, float %f_arg )
	%tmp2.5 = load void (i32, float)** getelementptr ([6 x void (i32, float)*]* @dispatch_tab, i32 0, i32 5), align 4
	tail call void %tmp2.5( i32 %i_arg, float %f_arg )
	ret void
}
