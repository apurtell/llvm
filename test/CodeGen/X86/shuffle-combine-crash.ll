; RUN: llc < %s -mtriple=x86_64-unknown-linux-gnu -mcpu=corei7 

; Verify that DAGCombiner does not crash when checking if it is
; safe to fold the shuffles in function @sample_test according to rule
;  (shuffle (shuffle A, Undef, M0), Undef, M1) -> (shuffle A, Undef, M2)
;
; The DAGCombiner avoids folding shuffles if
; the resulting shuffle dag node is not legal for the target.
; That means, the shuffle must have legal type and legal mask.
;
; Before, the DAGCombiner forgot to check if the resulting shuffle
; was legal. It instead just called method
; 'X86TargetLowering::isShuffleMaskLegal'; however, that was not enough since
; that method always expect to have a valid vector type in input.
; As a consequence, compiling the function below would have caused a crash.

define void @sample_test() {
  br i1 undef, label %5, label %1

; <label>:1                                       ; preds = %0
  %2 = load <4 x i8>* undef
  %3 = shufflevector <4 x i8> %2, <4 x i8> undef, <4 x i32> <i32 2, i32 2, i32 0, i32 0>
  %4 = shufflevector <4 x i8> %3, <4 x i8> undef, <4 x i32> <i32 2, i32 3, i32 0, i32 1>
  store <4 x i8> %4, <4 x i8>* undef
  br label %5

; <label>:5                                       ; preds = %1, %0
  ret void
}

