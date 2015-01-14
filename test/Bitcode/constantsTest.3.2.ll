; RUN:  llvm-dis < %s.bc| FileCheck %s

; constantsTest.3.2.ll.bc was generated by passing this file to llvm-as-3.2.
; The test checks that LLVM does not misread binary float instructions of
; older bitcode files.

;global variable address
; CHECK: @X = global i32 0
@X = global i32 0
; CHECK: @Y = global i32 1
@Y = global i32 1
; CHECK: @Z = global [2 x i32*] [i32* @X, i32* @Y]
@Z = global [2 x i32*] [i32* @X, i32* @Y]


define void @SimpleConstants(i32 %x) {
entry:
; null
; CHECK: store i32 %x, i32* null
  store i32 %x, i32* null
 
; boolean
; CHECK-NEXT: %res1 = fcmp true float 1.000000e+00, 1.000000e+00 
  %res1 = fcmp true float 1.0, 1.0
; CHECK-NEXT: %res2 = fcmp false float 1.000000e+00, 1.000000e+00
  %res2 = fcmp false float 1.0, 1.0

;integer
; CHECK-NEXT: %res3 = add i32 0, 0
  %res3 = add i32 0, 0

;float
; CHECK-NEXT: %res4 = fadd float 0.000000e+00, 0.000000e+00
  %res4 = fadd float 0.0, 0.0

  ret void
}

define void @ComplexConstants(<2 x i32> %x){
entry:
;constant structure
; CHECK: %res1 = extractvalue { i32, float } { i32 1, float 2.000000e+00 }, 0
  %res1 = extractvalue {i32, float} {i32 1, float 2.0}, 0
  
;const array
; CHECK-NEXT: %res2 = extractvalue [2 x i32] [i32 1, i32 2], 0
  %res2 = extractvalue [2 x i32] [i32 1, i32 2], 0
  
;const vector
; CHECK-NEXT: %res3 = add <2 x i32> <i32 1, i32 1>, <i32 1, i32 1>
  %res3 = add <2 x i32> <i32 1, i32 1>, <i32 1, i32 1>
  
;zeroinitializer
; CHECK-NEXT: %res4 = add <2 x i32> %x, zeroinitializer
  %res4 = add <2 x i32> %x, zeroinitializer
  
  ret void
}

define void @OtherConstants(i32 %x, i8* %Addr){
entry:
  ;undef
  ; CHECK: %res1 = add i32 %x, undef 
  %res1 = add i32 %x, undef
  
  ;poison
  ; CHECK-NEXT: %poison = sub nuw i32 0, 1
  %poison = sub nuw i32 0, 1
  
  ;address of basic block
  ; CHECK-NEXT: %res2 = icmp eq i8* blockaddress(@OtherConstants, %Next), null
  %res2 = icmp eq i8* blockaddress(@OtherConstants, %Next), null
  br label %Next
  Next: 
  ret void
}

define void @OtherConstants2(){
entry:
  ; CHECK: trunc i32 1 to i8
  trunc i32 1 to i8
  ; CHECK-NEXT: zext i8 1 to i32
  zext i8 1 to i32
  ; CHECK-NEXT: sext i8 1 to i32
  sext i8 1 to i32
  ; CHECK-NEXT: fptrunc double 1.000000e+00 to float
  fptrunc double 1.0 to float
  ; CHECK-NEXT: fpext float 1.000000e+00 to double
  fpext float 1.0 to double
  ; CHECK-NEXT: fptosi float 1.000000e+00 to i32
  fptosi float 1.0 to i32
  ; CHECK-NEXT: uitofp i32 1 to float
  uitofp i32 1 to float
  ; CHECK-NEXT: sitofp i32 -1 to float
  sitofp i32 -1 to float
  ; CHECK-NEXT: ptrtoint i32* @X to i32
  ptrtoint i32* @X to i32
  ; CHECK-NEXT: inttoptr i8 1 to i8*
  inttoptr i8 1 to i8*
  ; CHECK-NEXT: bitcast i32 1 to <2 x i16>
  bitcast i32 1 to <2 x i16>
  ; CHECK-NEXT: getelementptr i32* @X, i32 0
  getelementptr i32* @X, i32 0
  ; CHECK-NEXT: getelementptr inbounds i32* @X, i32 0
  getelementptr inbounds i32* @X, i32 0
  ; CHECK: select i1 true, i32 1, i32 0
  select i1 true ,i32 1, i32 0
  ; CHECK-NEXT: icmp eq i32 1, 0
  icmp eq i32 1, 0
  ; CHECK-NEXT: fcmp oeq float 1.000000e+00, 0.000000e+00
  fcmp oeq float 1.0, 0.0
  ; CHECK-NEXT: extractelement <2 x i32> <i32 1, i32 1>, i32 1
  extractelement <2 x i32> <i32 1, i32 1>, i32 1
  ; CHECK-NEXT: insertelement <2 x i32> <i32 1, i32 1>, i32 0, i32 1
  insertelement <2 x i32> <i32 1, i32 1>, i32 0, i32 1
  ; CHECK-NEXT: shufflevector <2 x i32> <i32 1, i32 1>, <2 x i32> zeroinitializer, <4 x i32> <i32 0, i32 2, i32 1, i32 3>
  shufflevector <2 x i32> <i32 1, i32 1>, <2 x i32> zeroinitializer, <4 x i32> <i32 0, i32 2, i32 1, i32 3>
  ; CHECK-NEXT: extractvalue { i32, float } { i32 1, float 2.000000e+00 }, 0
  extractvalue { i32, float } { i32 1, float 2.0 }, 0
  ; CHECK-NEXT: insertvalue { i32, float } { i32 1, float 2.000000e+00 }, i32 0, 0
  insertvalue { i32, float } { i32 1, float 2.0 }, i32 0, 0
  
  ret void
}