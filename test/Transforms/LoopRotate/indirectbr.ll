; RUN: opt < %s -S -loop-rotate -disable-output -verify-loop-info -verify-dom-info
; PR5502

define void @z80_do_opcodes() nounwind {
entry:
  br label %while.cond

while.cond:                                       ; preds = %end_opcode, %entry
  br label %while.body

while.body:                                       ; preds = %while.cond
  br label %indirectgoto

run_opcode:                                       ; preds = %indirectgoto
  %tmp276 = load i8* undef                        ; <i8> [#uses=1]
  br label %indirectgoto

if.else295:                                       ; preds = %divide_late
  br label %end_opcode

end_opcode:                                       ; preds = %indirectgoto, %sw.default42406, %sw.default, %if.else295
  %opcode.2 = phi i8 [ %opcode.0, %indirectgoto ], [ 0, %sw.default42406 ], [ undef, %sw.default ], [ %opcode.0, %if.else295 ] ; <i8> [#uses=0]
  switch i32 undef, label %while.cond [
    i32 221, label %sw.bb11691
    i32 253, label %sw.bb30351
  ]

sw.bb11691:                                       ; preds = %end_opcode
  br label %sw.default

sw.default:                                       ; preds = %sw.bb11691
  br label %end_opcode

sw.bb30351:                                       ; preds = %end_opcode
  br label %sw.default42406

sw.default42406:                                  ; preds = %sw.bb30351
  br label %end_opcode

indirectgoto:                                     ; preds = %run_opcode, %while.body
  %opcode.0 = phi i8 [ undef, %while.body ], [ %tmp276, %run_opcode ] ; <i8> [#uses=2]
  indirectbr i8* undef, [label %run_opcode, label %if.else295, label %end_opcode]
}
