@ RUN: llvm-mc -triple=thumbv6-apple-darwin -show-encoding < %s | FileCheck %s
  .syntax unified
  .globl _func

@ Check that the assembler can handle the documented syntax from the ARM ARM.
@ For complex constructs like shifter operands, check more thoroughly for them
@ once then spot check that following instructions accept the form generally.
@ This gives us good coverage while keeping the overall size of the test
@ more reasonable.


@ FIXME: Some 3-operand instructions have a 2-operand assembly syntax.

_func:
@ CHECK: _func

@------------------------------------------------------------------------------
@ ADC (register)
@------------------------------------------------------------------------------
        adcs r4, r6

@ CHECK: adcs	r4, r6                  @ encoding: [0x74,0x41]


@------------------------------------------------------------------------------
@ ADD (immediate)
@------------------------------------------------------------------------------
        adds r1, r2, #3
        adds r2, #3
        adds r2, #8

@ CHECK: adds	r1, r2, #3              @ encoding: [0xd1,0x1c]
@ CHECK: adds	r2, r2, #3              @ encoding: [0xd2,0x1c]
@ CHECK: adds	r2, #8                  @ encoding: [0x08,0x32]


@------------------------------------------------------------------------------
@ ADD (register)
@------------------------------------------------------------------------------
        adds r1, r2, r3
        add r2, r8

@ CHECK: adds	r1, r2, r3              @ encoding: [0xd1,0x18]
@ CHECK: add	r2, r8                  @ encoding: [0x42,0x44]


@------------------------------------------------------------------------------
@ FIXME: ADD (SP plus immediate)
@------------------------------------------------------------------------------
@------------------------------------------------------------------------------
@ FIXME: ADD (SP plus register)
@------------------------------------------------------------------------------


@------------------------------------------------------------------------------
@ ADR
@------------------------------------------------------------------------------
        adr r2, _baz

@ CHECK: adr	r2, _baz                @ encoding: [A,0xa2]
            @   fixup A - offset: 0, value: _baz, kind: fixup_thumb_adr_pcrel_10


@------------------------------------------------------------------------------
@ ASR (immediate)
@------------------------------------------------------------------------------
        asrs r2, r3, #32
        asrs r2, r3, #5
        asrs r2, r3, #1

@ CHECK: asrs	r2, r3, #32             @ encoding: [0x1a,0x10]
@ CHECK: asrs	r2, r3, #5              @ encoding: [0x5a,0x11]
@ CHECK: asrs	r2, r3, #1              @ encoding: [0x5a,0x10]


@------------------------------------------------------------------------------
@ ASR (register)
@------------------------------------------------------------------------------
        asrs r5, r2

@ CHECK: asrs	r5, r2                  @ encoding: [0x15,0x41]


@------------------------------------------------------------------------------
@ B
@------------------------------------------------------------------------------
        b _baz
        beq _bar

@ CHECK: b	_baz                    @ encoding: [A,0xe0'A']
             @   fixup A - offset: 0, value: _baz, kind: fixup_arm_thumb_br
@ CHECK: beq	_bar                    @ encoding: [A,0xd0]
             @   fixup A - offset: 0, value: _bar, kind: fixup_arm_thumb_bcc


@------------------------------------------------------------------------------
@ BICS
@------------------------------------------------------------------------------
        bics r1, r6

@ CHECK: bics	r1, r6                  @ encoding: [0xb1,0x43]


@------------------------------------------------------------------------------
@ BKPT
@------------------------------------------------------------------------------
        bkpt #0
        bkpt #255

@ CHECK: bkpt	#0                      @ encoding: [0x00,0xbe]
@ CHECK: bkpt	#255                    @ encoding: [0xff,0xbe]


@------------------------------------------------------------------------------
@ BL/BLX (immediate)
@------------------------------------------------------------------------------
        bl _bar
        blx _baz

@ CHECK: bl	_bar                    @ encoding: [A,0xf0'A',A,0xf8'A']
             @   fixup A - offset: 0, value: _bar, kind: fixup_arm_thumb_bl
@ CHECK: blx	_baz                    @ encoding: [A,0xf0'A',A,0xe8'A']
             @   fixup A - offset: 0, value: _baz, kind: fixup_arm_thumb_blx


@------------------------------------------------------------------------------
@ BLX (register)
@------------------------------------------------------------------------------
        blx r4

@ CHECK: blx	r4                      @ encoding: [0xa0,0x47]


@------------------------------------------------------------------------------
@ BX
@------------------------------------------------------------------------------
        bx r2

@ CHECK: bx	r2                      @ encoding: [0x10,0x47]


@------------------------------------------------------------------------------
@ CMN
@------------------------------------------------------------------------------

        cmn r5, r1

@ CHECK: cmn	r5, r1                  @ encoding: [0xcd,0x42]


@------------------------------------------------------------------------------
@ CMP
@------------------------------------------------------------------------------
        cmp r6, #32
        cmp r3, r4
        cmp r8, r1

@ CHECK: cmp	r6, #32                 @ encoding: [0x20,0x2e]
@ CHECK: cmp	r3, r4                  @ encoding: [0xa3,0x42]
@ CHECK: cmp	r8, r1                  @ encoding: [0x88,0x45]

@------------------------------------------------------------------------------
@ EOR
@------------------------------------------------------------------------------
        eors r4, r5

@ CHECK: eors	r4, r5                  @ encoding: [0x6c,0x40]


@------------------------------------------------------------------------------
@ LDM
@------------------------------------------------------------------------------
        ldm r3, {r0, r1, r2, r3, r4, r5, r6, r7}
        ldm r2!, {r1, r3, r4, r5, r7}
        ldm r1, {r1}

@ CHECK: ldm	r3, {r0, r1, r2, r3, r4, r5, r6, r7} @ encoding: [0xff,0xcb]
@ CHECK: ldm	r2!, {r1, r3, r4, r5, r7} @ encoding: [0xba,0xca]
@ CHECK: ldm	r1, {r1}                @ encoding: [0x02,0xc9]


@------------------------------------------------------------------------------
@ LDR (immediate)
@------------------------------------------------------------------------------
        ldr r1, [r5]
        ldr r2, [r6, #32]
        ldr r3, [r7, #124]
        ldr r1, [sp]
        ldr r2, [sp, #24]
        ldr r3, [sp, #1020]


@ CHECK: ldr	r1, [r5]                @ encoding: [0x29,0x68]
@ CHECK: ldr	r2, [r6, #32]           @ encoding: [0x32,0x6a]
@ CHECK: ldr	r3, [r7, #124]          @ encoding: [0xfb,0x6f]
@ CHECK: ldr	r1, [sp]                @ encoding: [0x00,0x99]
@ CHECK: ldr	r2, [sp, #24]           @ encoding: [0x06,0x9a]
@ CHECK: ldr	r3, [sp, #1020]         @ encoding: [0xff,0x9b]


@------------------------------------------------------------------------------
@ LDR (literal)
@------------------------------------------------------------------------------
        ldr r1, _foo

@ CHECK: ldr	r1, _foo                @ encoding: [A,0x49]
             @   fixup A - offset: 0, value: _foo, kind: fixup_arm_thumb_cp


@------------------------------------------------------------------------------
@ LDR (register)
@------------------------------------------------------------------------------
        ldr r1, [r2, r3]

@ CHECK: ldr	r1, [r2, r3]            @ encoding: [0xd1,0x58]


@------------------------------------------------------------------------------
@ LDRB (immediate)
@------------------------------------------------------------------------------
        ldrb r4, [r3]
        ldrb r5, [r6, #0]
        ldrb r6, [r7, #31]

@ CHECK: ldrb	r4, [r3]                @ encoding: [0x1c,0x78]
@ CHECK: ldrb	r5, [r6]                @ encoding: [0x35,0x78]
@ CHECK: ldrb	r6, [r7, #31]           @ encoding: [0xfe,0x7f]


@------------------------------------------------------------------------------
@ LDRB (register)
@------------------------------------------------------------------------------
        ldrb r6, [r4, r5]

@ CHECK: ldrb	r6, [r4, r5]            @ encoding: [0x66,0x5d]


@------------------------------------------------------------------------------
@ LDRH (immediate)
@------------------------------------------------------------------------------
        ldrh r3, [r3]
        ldrh r4, [r6, #2]
        ldrh r5, [r7, #62]

@ CHECK: ldrh	r3, [r3]                @ encoding: [0x1b,0x88]
@ CHECK: ldrh	r4, [r6, #2]            @ encoding: [0x74,0x88]
@ CHECK: ldrh	r5, [r7, #62]           @ encoding: [0xfd,0x8f]


@------------------------------------------------------------------------------
@ LDRH (register)
@------------------------------------------------------------------------------
        ldrh r6, [r2, r6]

@ CHECK: ldrh	r6, [r2, r6]            @ encoding: [0x96,0x5b]


@------------------------------------------------------------------------------
@ LDRSB/LDRSH
@------------------------------------------------------------------------------
        ldrsb r6, [r2, r6]
        ldrsh r3, [r7, r1]

@ CHECK: ldrsb	r6, [r2, r6]            @ encoding: [0x96,0x57]
@ CHECK: ldrsh	r3, [r7, r1]            @ encoding: [0x7b,0x5e]


@------------------------------------------------------------------------------
@ LSL (immediate)
@------------------------------------------------------------------------------
        lsls r4, r5, #0
        lsls r4, r5, #4

@ CHECK: lsls	r4, r5, #0              @ encoding: [0x2c,0x00]
@ CHECK: lsls	r4, r5, #4              @ encoding: [0x2c,0x01]


@------------------------------------------------------------------------------
@ LSL (register)
@------------------------------------------------------------------------------
        lsls r2, r6

@ CHECK: lsls	r2, r6                  @ encoding: [0xb2,0x40]


@------------------------------------------------------------------------------
@ LSR (immediate)
@------------------------------------------------------------------------------
        lsrs r1, r3, #1
        lsrs r1, r3, #32

@ CHECK: lsrs	r1, r3, #1              @ encoding: [0x59,0x08]
@ CHECK: lsrs	r1, r3, #32             @ encoding: [0x19,0x08]


@------------------------------------------------------------------------------
@ LSR (register)
@------------------------------------------------------------------------------
        lsrs r2, r6

@ CHECK: lsrs	r2, r6                  @ encoding: [0xf2,0x40]


@------------------------------------------------------------------------------
@ MOV (immediate)
@------------------------------------------------------------------------------
        movs r2, #0
        movs r2, #255
        movs r2, #23

@ CHECK: movs	r2, #0                  @ encoding: [0x00,0x22]
@ CHECK: movs	r2, #255                @ encoding: [0xff,0x22]
@ CHECK: movs	r2, #23                 @ encoding: [0x17,0x22]


@------------------------------------------------------------------------------
@ MOV (register)
@------------------------------------------------------------------------------
        mov r3, r4
        movs r1, r3

@ CHECK: mov	r3, r4                  @ encoding: [0x23,0x46]
@ CHECK: movs	r1, r3                  @ encoding: [0x19,0x00]


@------------------------------------------------------------------------------
@ MUL
@------------------------------------------------------------------------------
        muls r1, r2, r1
        muls r3, r4

@ CHECK: muls	r1, r2, r1              @ encoding: [0x51,0x43]
@ CHECK: muls	r3, r4, r3              @ encoding: [0x63,0x43]


@------------------------------------------------------------------------------
@ MVN
@------------------------------------------------------------------------------
        mvns r6, r3

@ CHECK: mvns	r6, r3                  @ encoding: [0xde,0x43]


@------------------------------------------------------------------------------
@ NEG
@------------------------------------------------------------------------------
        negs r3, r4

@ CHECK: rsbs	r3, r4, #0              @ encoding: [0x63,0x42]


@------------------------------------------------------------------------------
@ NOP
@------------------------------------------------------------------------------
        nop

@ CHECK: nop                            @ encoding: [0xc0,0x46]


@------------------------------------------------------------------------------
@ ORR
@------------------------------------------------------------------------------
        orrs  r3, r4

@ CHECK-ERRORS: 	orrs	r3, r4                  @ encoding: [0x23,0x43]


@------------------------------------------------------------------------------
@ POP
@------------------------------------------------------------------------------
        pop {r2, r3, r6}

@ CHECK: pop	{r2, r3, r6}            @ encoding: [0x4c,0xbc]
