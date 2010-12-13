; RUN: opt < %s -simplifycfg -S | FileCheck %s

declare void @foo1()

declare void @foo2()

define void @test1(i32 %V) {
        %C1 = icmp eq i32 %V, 4         ; <i1> [#uses=1]
        %C2 = icmp eq i32 %V, 17                ; <i1> [#uses=1]
        %CN = or i1 %C1, %C2            ; <i1> [#uses=1]
        br i1 %CN, label %T, label %F
T:              ; preds = %0
        call void @foo1( )
        ret void
F:              ; preds = %0
        call void @foo2( )
        ret void
; CHECK: @test1
; CHECK:  switch i32 %V, label %F [
; CHECK:    i32 17, label %T
; CHECK:    i32 4, label %T
; CHECK:  ]
}

define void @test2(i32 %V) {
        %C1 = icmp ne i32 %V, 4         ; <i1> [#uses=1]
        %C2 = icmp ne i32 %V, 17                ; <i1> [#uses=1]
        %CN = and i1 %C1, %C2           ; <i1> [#uses=1]
        br i1 %CN, label %T, label %F
T:              ; preds = %0
        call void @foo1( )
        ret void
F:              ; preds = %0
        call void @foo2( )
        ret void
; CHECK: @test2
; CHECK:  switch i32 %V, label %T [
; CHECK:    i32 17, label %F
; CHECK:    i32 4, label %F
; CHECK:  ]
}

define void @test3(i32 %V) {
        %C1 = icmp eq i32 %V, 4         ; <i1> [#uses=1]
        br i1 %C1, label %T, label %N
N:              ; preds = %0
        %C2 = icmp eq i32 %V, 17                ; <i1> [#uses=1]
        br i1 %C2, label %T, label %F
T:              ; preds = %N, %0
        call void @foo1( )
        ret void
F:              ; preds = %N
        call void @foo2( )
        ret void

; CHECK: @test3
; CHECK: switch i32 %V, label %F [
; CHECK:     i32 4, label %T
; CHECK:     i32 17, label %T
; CHECK:   ]
}



define i32 @test4(i8 zeroext %c) nounwind ssp noredzone {
entry:
  %cmp = icmp eq i8 %c, 62
  br i1 %cmp, label %lor.end, label %lor.lhs.false

lor.lhs.false:                                    ; preds = %entry
  %cmp4 = icmp eq i8 %c, 34
  br i1 %cmp4, label %lor.end, label %lor.rhs

lor.rhs:                                          ; preds = %lor.lhs.false
  %cmp8 = icmp eq i8 %c, 92
  br label %lor.end

lor.end:                                          ; preds = %lor.rhs, %lor.lhs.false, %entry
  %0 = phi i1 [ true, %lor.lhs.false ], [ true, %entry ], [ %cmp8, %lor.rhs ]
  %lor.ext = zext i1 %0 to i32
  ret i32 %lor.ext
  
; CHECK: @test4
; CHECK:  switch i8 %c, label %lor.rhs [
; CHECK:    i8 62, label %lor.end
; CHECK:    i8 34, label %lor.end
; CHECK:    i8 92, label %lor.end
; CHECK:  ]
}

define i32 @test5(i8 zeroext %c) nounwind ssp noredzone {
entry:
  switch i8 %c, label %lor.rhs [
    i8 62, label %lor.end
    i8 34, label %lor.end
    i8 92, label %lor.end
  ]

lor.rhs:                                          ; preds = %entry
  %V = icmp eq i8 %c, 92
  br label %lor.end

lor.end:                                          ; preds = %entry, %entry, %entry, %lor.rhs
  %0 = phi i1 [ true, %entry ], [ %V, %lor.rhs ], [ true, %entry ], [ true, %entry ]
  %lor.ext = zext i1 %0 to i32
  ret i32 %lor.ext
; CHECK: @test5
; CHECK:  switch i8 %c, label %lor.rhs [
; CHECK:    i8 62, label %lor.end
; CHECK:    i8 34, label %lor.end
; CHECK:    i8 92, label %lor.end
; CHECK:  ]
}


define i1 @test6({ i32, i32 }* %I) {
entry:
        %tmp.1.i = getelementptr { i32, i32 }* %I, i64 0, i32 1         ; <i32*> [#uses=1]
        %tmp.2.i = load i32* %tmp.1.i           ; <i32> [#uses=6]
        %tmp.2 = icmp eq i32 %tmp.2.i, 14               ; <i1> [#uses=1]
        br i1 %tmp.2, label %shortcirc_done.4, label %shortcirc_next.0
shortcirc_next.0:               ; preds = %entry
        %tmp.6 = icmp eq i32 %tmp.2.i, 15               ; <i1> [#uses=1]
        br i1 %tmp.6, label %shortcirc_done.4, label %shortcirc_next.1
shortcirc_next.1:               ; preds = %shortcirc_next.0
        %tmp.11 = icmp eq i32 %tmp.2.i, 16              ; <i1> [#uses=1]
        br i1 %tmp.11, label %shortcirc_done.4, label %shortcirc_next.2
shortcirc_next.2:               ; preds = %shortcirc_next.1
        %tmp.16 = icmp eq i32 %tmp.2.i, 17              ; <i1> [#uses=1]
        br i1 %tmp.16, label %shortcirc_done.4, label %shortcirc_next.3
shortcirc_next.3:               ; preds = %shortcirc_next.2
        %tmp.21 = icmp eq i32 %tmp.2.i, 18              ; <i1> [#uses=1]
        br i1 %tmp.21, label %shortcirc_done.4, label %shortcirc_next.4
shortcirc_next.4:               ; preds = %shortcirc_next.3
        %tmp.26 = icmp eq i32 %tmp.2.i, 19              ; <i1> [#uses=1]
        br label %UnifiedReturnBlock
shortcirc_done.4:               ; preds = %shortcirc_next.3, %shortcirc_next.2, %shortcirc_next.1, %shortcirc_next.0, %entry
        br label %UnifiedReturnBlock
UnifiedReturnBlock:             ; preds = %shortcirc_done.4, %shortcirc_next.4
        %UnifiedRetVal = phi i1 [ %tmp.26, %shortcirc_next.4 ], [ true, %shortcirc_done.4 ]             ; <i1> [#uses=1]
        ret i1 %UnifiedRetVal
        
; CHECK: @test6
; CHECK:   switch i32 %tmp.2.i, label %shortcirc_next.4 [
; CHECK:       i32 14, label %UnifiedReturnBlock
; CHECK:       i32 15, label %UnifiedReturnBlock
; CHECK:       i32 16, label %UnifiedReturnBlock
; CHECK:       i32 17, label %UnifiedReturnBlock
; CHECK:       i32 18, label %UnifiedReturnBlock
; CHECK:       i32 19, label %switch.edge
; CHECK:     ]
}

define void @test7(i8 zeroext %c, i32 %x) nounwind ssp noredzone {
entry:
  %cmp = icmp ult i32 %x, 32
  %cmp4 = icmp eq i8 %c, 97
  %or.cond = or i1 %cmp, %cmp4
  %cmp9 = icmp eq i8 %c, 99
  %or.cond11 = or i1 %or.cond, %cmp9
  br i1 %or.cond11, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  tail call void @foo1() nounwind noredzone
  ret void

if.end:                                           ; preds = %entry
  ret void
  
; CHECK: @test7
; CHECK:   %cmp = icmp ult i32 %x, 32
; CHECK:   br i1 %cmp, label %if.then, label %switch.early.test
; CHECK: switch.early.test:
; CHECK:   switch i8 %c, label %if.end [
; CHECK:     i8 99, label %if.then
; CHECK:     i8 97, label %if.then
; CHECK:   ]
}

define i32 @test8(i8 zeroext %c, i32 %x, i1 %C) nounwind ssp noredzone {
entry:
  br i1 %C, label %N, label %if.then
N:
  %cmp = icmp ult i32 %x, 32
  %cmp4 = icmp eq i8 %c, 97
  %or.cond = or i1 %cmp, %cmp4
  %cmp9 = icmp eq i8 %c, 99
  %or.cond11 = or i1 %or.cond, %cmp9
  br i1 %or.cond11, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %A = phi i32 [0, %entry], [42, %N]
  tail call void @foo1() nounwind noredzone
  ret i32 %A

if.end:                                           ; preds = %entry
  ret i32 0
  
; CHECK: @test8
; CHECK: switch.early.test:
; CHECK:   switch i8 %c, label %if.end [
; CHECK:     i8 99, label %if.then
; CHECK:     i8 97, label %if.then
; CHECK:   ]
; CHECK:   %A = phi i32 [ 0, %entry ], [ 42, %switch.early.test ], [ 42, %N ], [ 42, %switch.early.test ]
}

;; This is "Example 7" from http://blog.regehr.org/archives/320
define i32 @test9(i8 zeroext %c) nounwind ssp noredzone {
entry:
  %cmp = icmp ult i8 %c, 33
  br i1 %cmp, label %lor.end, label %lor.lhs.false

lor.lhs.false:                                    ; preds = %entry
  %cmp4 = icmp eq i8 %c, 46
  br i1 %cmp4, label %lor.end, label %lor.lhs.false6

lor.lhs.false6:                                   ; preds = %lor.lhs.false
  %cmp9 = icmp eq i8 %c, 44
  br i1 %cmp9, label %lor.end, label %lor.lhs.false11

lor.lhs.false11:                                  ; preds = %lor.lhs.false6
  %cmp14 = icmp eq i8 %c, 58
  br i1 %cmp14, label %lor.end, label %lor.lhs.false16

lor.lhs.false16:                                  ; preds = %lor.lhs.false11
  %cmp19 = icmp eq i8 %c, 59
  br i1 %cmp19, label %lor.end, label %lor.lhs.false21

lor.lhs.false21:                                  ; preds = %lor.lhs.false16
  %cmp24 = icmp eq i8 %c, 60
  br i1 %cmp24, label %lor.end, label %lor.lhs.false26

lor.lhs.false26:                                  ; preds = %lor.lhs.false21
  %cmp29 = icmp eq i8 %c, 62
  br i1 %cmp29, label %lor.end, label %lor.lhs.false31

lor.lhs.false31:                                  ; preds = %lor.lhs.false26
  %cmp34 = icmp eq i8 %c, 34
  br i1 %cmp34, label %lor.end, label %lor.lhs.false36

lor.lhs.false36:                                  ; preds = %lor.lhs.false31
  %cmp39 = icmp eq i8 %c, 92
  br i1 %cmp39, label %lor.end, label %lor.rhs

lor.rhs:                                          ; preds = %lor.lhs.false36
  %cmp43 = icmp eq i8 %c, 39
  br label %lor.end

lor.end:                                          ; preds = %lor.rhs, %lor.lhs.false36, %lor.lhs.false31, %lor.lhs.false26, %lor.lhs.false21, %lor.lhs.false16, %lor.lhs.false11, %lor.lhs.false6, %lor.lhs.false, %entry
  %0 = phi i1 [ true, %lor.lhs.false36 ], [ true, %lor.lhs.false31 ], [ true, %lor.lhs.false26 ], [ true, %lor.lhs.false21 ], [ true, %lor.lhs.false16 ], [ true, %lor.lhs.false11 ], [ true, %lor.lhs.false6 ], [ true, %lor.lhs.false ], [ true, %entry ], [ %cmp43, %lor.rhs ]
  %conv46 = zext i1 %0 to i32
  ret i32 %conv46
  
; CHECK: @test9
; CHECK:   %cmp = icmp ult i8 %c, 33
; CHECK:   br i1 %cmp, label %lor.end, label %switch.early.test

; CHECK: switch.early.test:
; CHECK:   switch i8 %c, label %lor.rhs [
; CHECK:     i8 46, label %lor.end
; CHECK:     i8 44, label %lor.end
; CHECK:     i8 58, label %lor.end
; CHECK:     i8 59, label %lor.end
; CHECK:     i8 60, label %lor.end
; CHECK:     i8 62, label %lor.end
; CHECK:     i8 34, label %lor.end
; CHECK:     i8 92, label %lor.end
; CHECK:     i8 39, label %lor.end
; CHECK:   ]
}

define i32 @test10(i32 %mode, i1 %Cond) {
  %A = icmp ne i32 %mode, 0
  %B = icmp ne i32 %mode, 51
  %C = and i1 %A, %B
  %D = and i1 %C, %Cond
  br i1 %D, label %T, label %F
T:
  ret i32 123
F:
  ret i32 324

; CHECK: @test10
; CHECK:  br i1 %Cond, label %switch.early.test, label %F
; CHECK:switch.early.test:
; CHECK:  switch i32 %mode, label %T [
; CHECK:    i32 51, label %F
; CHECK:    i32 0, label %F
; CHECK:  ]
}

; PR8780
define i32 @test11(i32 %bar) nounwind {
entry:
  %cmp = icmp eq i32 %bar, 4
  %cmp2 = icmp eq i32 %bar, 35
  %or.cond = or i1 %cmp, %cmp2
  %cmp5 = icmp eq i32 %bar, 53
  %or.cond1 = or i1 %or.cond, %cmp5
  %cmp8 = icmp eq i32 %bar, 24
  %or.cond2 = or i1 %or.cond1, %cmp8
  %cmp11 = icmp eq i32 %bar, 23
  %or.cond3 = or i1 %or.cond2, %cmp11
  %cmp14 = icmp eq i32 %bar, 55
  %or.cond4 = or i1 %or.cond3, %cmp14
  %cmp17 = icmp eq i32 %bar, 12
  %or.cond5 = or i1 %or.cond4, %cmp17
  %cmp20 = icmp eq i32 %bar, 35
  %or.cond6 = or i1 %or.cond5, %cmp20
  br i1 %or.cond6, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %return

if.end:                                           ; preds = %entry
  br label %return

return:                                           ; preds = %if.end, %if.then
  %retval.0 = phi i32 [ 1, %if.then ], [ 0, %if.end ]
  ret i32 %retval.0

; CHECK: @test11
; CHECK: switch i32 %bar, label %if.end [
; CHECK:   i32 55, label %return
; CHECK:   i32 53, label %return
; CHECK:   i32 35, label %return
; CHECK:   i32 24, label %return
; CHECK:   i32 23, label %return
; CHECK:   i32 12, label %return
; CHECK:   i32 4, label %return
; CHECK: ]
}
