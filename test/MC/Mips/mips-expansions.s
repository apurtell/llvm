# RUN: llvm-mc %s -triple=mipsel-unknown-linux -show-encoding -mcpu=mips32r2 | \
# RUN:   FileCheck %s --check-prefix=CHECK-LE
# RUN: llvm-mc %s -triple=mips-unknown-linux -show-encoding -mcpu=mips32r2 | \
# RUN:   FileCheck %s --check-prefix=CHECK-BE

# Check that the IAS expands macro instructions in the same way as GAS.

# Load immediate, done by MipsAsmParser::expandLoadImm():
  li $5, 123
# CHECK-LE:     ori     $5, $zero, 123   # encoding: [0x7b,0x00,0x05,0x34]
  li $6, -2345
# CHECK-LE:     addiu   $6, $zero, -2345 # encoding: [0xd7,0xf6,0x06,0x24]
  li $7, 65538
# CHECK-LE:     lui     $7, 1            # encoding: [0x01,0x00,0x07,0x3c]
# CHECK-LE:     ori     $7, $7, 2        # encoding: [0x02,0x00,0xe7,0x34]
  li $8, ~7
# CHECK-LE:     addiu   $8, $zero, -8    # encoding: [0xf8,0xff,0x08,0x24]
  li $9, 0x10000
# CHECK-LE:     lui     $9, 1            # encoding: [0x01,0x00,0x09,0x3c]
# CHECK-LE-NOT: ori $9, $9, 0            # encoding: [0x00,0x00,0x29,0x35]
  li $10, ~(0x101010)
# CHECK-LE:     lui     $10, 65519       # encoding: [0xef,0xff,0x0a,0x3c]
# CHECK-LE:     ori     $10, $10, 61423  # encoding: [0xef,0xef,0x4a,0x35]

# Load address, done by MipsAsmParser::expandLoadAddressReg()
# and MipsAsmParser::expandLoadAddressImm():
  la $4, 20
# CHECK-LE: ori     $4, $zero, 20       # encoding: [0x14,0x00,0x04,0x34]
  la $7, 65538
# CHECK-LE: lui     $7, 1               # encoding: [0x01,0x00,0x07,0x3c]
# CHECK-LE: ori     $7, $7, 2           # encoding: [0x02,0x00,0xe7,0x34]
  la $4, 20($5)
# CHECK-LE: ori     $4, $5, 20          # encoding: [0x14,0x00,0xa4,0x34]
  la $7, 65538($8)
# CHECK-LE: lui     $7, 1               # encoding: [0x01,0x00,0x07,0x3c]
# CHECK-LE: ori     $7, $7, 2           # encoding: [0x02,0x00,0xe7,0x34]
# CHECK-LE: addu    $7, $7, $8          # encoding: [0x21,0x38,0xe8,0x00]
  la $8, 1f
# CHECK-LE: lui     $8, %hi($tmp0)      # encoding: [A,A,0x08,0x3c]
# CHECK-LE:                             #   fixup A - offset: 0, value: ($tmp0)@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: ori     $8, $8, %lo($tmp0)  # encoding: [A,A,0x08,0x35]
# CHECK-LE:                             #   fixup A - offset: 0, value: ($tmp0)@ABS_LO, kind: fixup_Mips_LO16
  la $8, symbol
# CHECK-LE: lui     $8, %hi(symbol)     # encoding: [A,A,0x08,0x3c]
# CHECK-LE:                             #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: ori     $8, $8, %lo(symbol) # encoding: [A,A,0x08,0x35]
# CHECK-LE:                             #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16
  la $8, symbol($9)
# CHECK-LE: lui  $8, %hi(symbol)        # encoding: [A,A,0x08,0x3c]
# CHECK-LE:                             #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: ori  $8, $8, %lo(symbol)    # encoding: [A,A,0x08,0x35]
# CHECK-LE:                             #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16
# CHECK-LE: addu $8, $8, $9             # encoding: [0x21,0x40,0x09,0x01]
  la $8, symbol($8)
# CHECK-LE: lui  $1, %hi(symbol)        # encoding: [A,A,0x01,0x3c]
# CHECK-LE:                             #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: ori  $1, $1, %lo(symbol)    # encoding: [A,A,0x21,0x34]
# CHECK-LE:                             #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16
# CHECK-LE: addu $8, $1, $8             # encoding: [0x21,0x40,0x28,0x00]
  la $8, 20($8)
# CHECK-LE: ori  $8, $8, 20             # encoding: [0x14,0x00,0x08,0x35]
  la $8, 65538($8)
# CHECK-LE: lui  $1, 1                  # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: ori  $1, $1, 2              # encoding: [0x02,0x00,0x21,0x34]
# CHECK-LE: addu $8, $1, $8             # encoding: [0x21,0x40,0x28,0x00]

# LW/SW and LDC1/SDC1 of symbol address, done by MipsAsmParser::expandMemInst():
  .set noat
  lw $10, symbol($4)
# CHECK-LE: lui     $10, %hi(symbol)        # encoding: [A,A,0x0a,0x3c]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: addu    $10, $10, $4            # encoding: [0x21,0x50,0x44,0x01]
# CHECK-LE: lw      $10, %lo(symbol)($10)   # encoding: [A,A,0x4a,0x8d]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16
  .set at
  sw $10, symbol($9)
# CHECK-LE: lui     $1, %hi(symbol)         # encoding: [A,A,0x01,0x3c]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: addu    $1, $1, $9              # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: sw      $10, %lo(symbol)($1)    # encoding: [A,A,0x2a,0xac]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16

  lw $8, 1f
# CHECK-LE: lui $8, %hi($tmp0)              # encoding: [A,A,0x08,0x3c]
# CHECK-LE:                                 #   fixup A - offset: 0, value: ($tmp0)@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: lw  $8, %lo($tmp0)($8)          # encoding: [A,A,0x08,0x8d]
# CHECK-LE:                                 #   fixup A - offset: 0, value: ($tmp0)@ABS_LO, kind: fixup_Mips_LO16
  sw $8, 1f
# CHECK-LE: lui $1, %hi($tmp0)              # encoding: [A,A,0x01,0x3c]
# CHECK-LE:                                 #   fixup A - offset: 0, value: ($tmp0)@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE: sw  $8, %lo($tmp0)($1)          # encoding: [A,A,0x28,0xac]
# CHECK-LE:                                 #   fixup A - offset: 0, value: ($tmp0)@ABS_LO, kind: fixup_Mips_LO16

  lw $10, 655483($4)
# CHECK-LE: lui     $10, 10                 # encoding: [0x0a,0x00,0x0a,0x3c]
# CHECK-LE: addu    $10, $10, $4            # encoding: [0x21,0x50,0x44,0x01]
# CHECK-LE: lw      $10, 123($10)           # encoding: [0x7b,0x00,0x4a,0x8d]
  sw $10, 123456($9)
# CHECK-LE: lui     $1, 2                   # encoding: [0x02,0x00,0x01,0x3c]
# CHECK-LE: addu    $1, $1, $9              # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: sw      $10, 57920($1)          # encoding: [0x40,0xe2,0x2a,0xac]

  lw $8, symbol
# CHECK-LE:     lui     $8, %hi(symbol)     # encoding: [A,A,0x08,0x3c]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE-NOT: move    $8, $8              # encoding: [0x21,0x40,0x00,0x01]
# CHECK-LE:     lw      $8, %lo(symbol)($8) # encoding: [A,A,0x08,0x8d]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16
  sw $8, symbol
# CHECK-LE:     lui     $1, %hi(symbol)     # encoding: [A,A,0x01,0x3c]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_HI, kind: fixup_Mips_HI16
# CHECK-LE-NOT: move    $1, $1              # encoding: [0x21,0x08,0x20,0x00]
# CHECK-LE:     sw      $8, %lo(symbol)($1) # encoding: [A,A,0x28,0xac]
# CHECK-LE:                                 #   fixup A - offset: 0, value: symbol@ABS_LO, kind: fixup_Mips_LO16

  ldc1 $f0, symbol
# CHECK-LE: lui     $1, %hi(symbol)
# CHECK-LE: ldc1    $f0, %lo(symbol)($1)
  sdc1 $f0, symbol
# CHECK-LE: lui     $1, %hi(symbol)
# CHECK-LE: sdc1    $f0, %lo(symbol)($1)

# Test BNE with an immediate as the 2nd operand.
  bne $2, 0, 1332
# CHECK-LE: bnez  $2, 1332          # encoding: [0x4d,0x01,0x40,0x14]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  bne $2, 123, 1332
# CHECK-LE: ori   $1, $zero, 123    # encoding: [0x7b,0x00,0x01,0x34]
# CHECK-LE: bne   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x14]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  bne $2, -2345, 1332
# CHECK-LE: addiu $1, $zero, -2345  # encoding: [0xd7,0xf6,0x01,0x24]
# CHECK-LE: bne   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x14]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  bne $2, 65538, 1332
# CHECK-LE: lui   $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: ori   $1, $1, 2         # encoding: [0x02,0x00,0x21,0x34]
# CHECK-LE: bne   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x14]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  bne $2, ~7, 1332
# CHECK-LE: addiu $1, $zero, -8     # encoding: [0xf8,0xff,0x01,0x24]
# CHECK-LE: bne   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x14]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  bne $2, 0x10000, 1332
# CHECK-LE: lui   $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: bne   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x14]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

# Test BEQ with an immediate as the 2nd operand.
  beq $2, 0, 1332
# CHECK-LE: beqz  $2, 1332          # encoding: [0x4d,0x01,0x40,0x10]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  beq $2, 123, 1332
# CHECK-LE: ori   $1, $zero, 123    # encoding: [0x7b,0x00,0x01,0x34]
# CHECK-LE: beq   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x10]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  beq $2, -2345, 1332
# CHECK-LE: addiu $1, $zero, -2345  # encoding: [0xd7,0xf6,0x01,0x24]
# CHECK-LE: beq   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x10]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  beq $2, 65538, 1332
# CHECK-LE: lui   $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: ori   $1, $1, 2         # encoding: [0x02,0x00,0x21,0x34]
# CHECK-LE: beq   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x10]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  beq $2, ~7, 1332
# CHECK-LE: addiu $1, $zero, -8     # encoding: [0xf8,0xff,0x01,0x24]
# CHECK-LE: beq   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x10]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

  beq $2, 0x10000, 1332
# CHECK-LE: lui   $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: beq   $2, $1, 1332      # encoding: [0x4d,0x01,0x41,0x10]
# CHECK-LE: nop                     # encoding: [0x00,0x00,0x00,0x00]

# Test ULHU with immediate operand.
  ulhu $8, 0
# CHECK-BE: lbu  $1, 0($zero)      # encoding: [0x90,0x01,0x00,0x00]
# CHECK-BE: lbu  $8, 1($zero)      # encoding: [0x90,0x08,0x00,0x01]
# CHECK-BE: sll  $1, $1, 8         # encoding: [0x00,0x01,0x0a,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lbu  $1, 1($zero)      # encoding: [0x01,0x00,0x01,0x90]
# CHECK-LE: lbu  $8, 0($zero)      # encoding: [0x00,0x00,0x08,0x90]
# CHECK-LE: sll  $1, $1, 8         # encoding: [0x00,0x0a,0x01,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 2
# CHECK-BE: lbu  $1, 2($zero)      # encoding: [0x90,0x01,0x00,0x02]
# CHECK-BE: lbu  $8, 3($zero)      # encoding: [0x90,0x08,0x00,0x03]
# CHECK-BE: sll  $1, $1, 8         # encoding: [0x00,0x01,0x0a,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lbu  $1, 3($zero)      # encoding: [0x03,0x00,0x01,0x90]
# CHECK-LE: lbu  $8, 2($zero)      # encoding: [0x02,0x00,0x08,0x90]
# CHECK-LE: sll  $1, $1, 8         # encoding: [0x00,0x0a,0x01,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 0x8000
# CHECK-BE: ori  $1, $zero, 32768  # encoding: [0x34,0x01,0x80,0x00]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: ori  $1, $zero, 32768  # encoding: [0x00,0x80,0x01,0x34]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, -0x8000
# CHECK-BE: lbu  $1, -32768($zero) # encoding: [0x90,0x01,0x80,0x00]
# CHECK-BE: lbu  $8, -32767($zero) # encoding: [0x90,0x08,0x80,0x01]
# CHECK-BE: sll  $1, $1, 8         # encoding: [0x00,0x01,0x0a,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lbu  $1, -32767($zero) # encoding: [0x01,0x80,0x01,0x90]
# CHECK-LE: lbu  $8, -32768($zero) # encoding: [0x00,0x80,0x08,0x90]
# CHECK-LE: sll  $1, $1, 8         # encoding: [0x00,0x0a,0x01,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 0x10000
# CHECK-BE: lui  $1, 1             # encoding: [0x3c,0x01,0x00,0x01]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lui  $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 0x18888
# CHECK-BE: lui  $1, 1             # encoding: [0x3c,0x01,0x00,0x01]
# CHECK-BE: ori  $1, $1, 34952     # encoding: [0x34,0x21,0x88,0x88]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lui  $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: ori  $1, $1, 34952     # encoding: [0x88,0x88,0x21,0x34]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, -32769
# CHECK-BE: lui  $1, 65535         # encoding: [0x3c,0x01,0xff,0xff]
# CHECK-BE: ori  $1, $1, 32767     # encoding: [0x34,0x21,0x7f,0xff]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lui  $1, 65535         # encoding: [0xff,0xff,0x01,0x3c]
# CHECK-LE: ori  $1, $1, 32767     # encoding: [0xff,0x7f,0x21,0x34]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 32767
# CHECK-BE: ori  $1, $zero, 32767  # encoding: [0x34,0x01,0x7f,0xff]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: ori  $1, $zero, 32767  # encoding: [0xff,0x7f,0x01,0x34]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

# Test ULHU with immediate offset and a source register operand.
  ulhu $8, 0($9)
# CHECK-BE: lbu  $1, 0($9)         # encoding: [0x91,0x21,0x00,0x00]
# CHECK-BE: lbu  $8, 1($9)         # encoding: [0x91,0x28,0x00,0x01]
# CHECK-BE: sll  $1, $1, 8         # encoding: [0x00,0x01,0x0a,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lbu  $1, 1($9)         # encoding: [0x01,0x00,0x21,0x91]
# CHECK-LE: lbu  $8, 0($9)         # encoding: [0x00,0x00,0x28,0x91]
# CHECK-LE: sll  $1, $1, 8         # encoding: [0x00,0x0a,0x01,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 2($9)
# CHECK-BE: lbu  $1, 2($9)         # encoding: [0x91,0x21,0x00,0x02]
# CHECK-BE: lbu  $8, 3($9)         # encoding: [0x91,0x28,0x00,0x03]
# CHECK-BE: sll  $1, $1, 8         # encoding: [0x00,0x01,0x0a,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lbu  $1, 3($9)         # encoding: [0x03,0x00,0x21,0x91]
# CHECK-LE: lbu  $8, 2($9)         # encoding: [0x02,0x00,0x28,0x91]
# CHECK-LE: sll  $1, $1, 8         # encoding: [0x00,0x0a,0x01,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 0x8000($9)
# CHECK-BE: ori  $1, $zero, 32768  # encoding: [0x34,0x01,0x80,0x00]
# CHECK-BE: addu $1, $1, $9        # encoding: [0x00,0x29,0x08,0x21]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: ori  $1, $zero, 32768  # encoding: [0x00,0x80,0x01,0x34]
# CHECK-LE: addu $1, $1, $9        # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, -0x8000($9)
# CHECK-BE: lbu  $1, -32768($9)    # encoding: [0x91,0x21,0x80,0x00]
# CHECK-BE: lbu  $8, -32767($9)    # encoding: [0x91,0x28,0x80,0x01]
# CHECK-BE: sll  $1, $1, 8         # encoding: [0x00,0x01,0x0a,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lbu  $1, -32767($9)    # encoding: [0x01,0x80,0x21,0x91]
# CHECK-LE: lbu  $8, -32768($9)    # encoding: [0x00,0x80,0x28,0x91]
# CHECK-LE: sll  $1, $1, 8         # encoding: [0x00,0x0a,0x01,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 0x10000($9)
# CHECK-BE: lui  $1, 1             # encoding: [0x3c,0x01,0x00,0x01]
# CHECK-BE: addu $1, $1, $9        # encoding: [0x00,0x29,0x08,0x21]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lui  $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: addu $1, $1, $9        # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 0x18888($9)
# CHECK-BE: lui  $1, 1             # encoding: [0x3c,0x01,0x00,0x01]
# CHECK-BE: ori  $1, $1, 34952     # encoding: [0x34,0x21,0x88,0x88]
# CHECK-BE: addu $1, $1, $9        # encoding: [0x00,0x29,0x08,0x21]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lui  $1, 1             # encoding: [0x01,0x00,0x01,0x3c]
# CHECK-LE: ori  $1, $1, 34952     # encoding: [0x88,0x88,0x21,0x34]
# CHECK-LE: addu $1, $1, $9        # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, -32769($9)
# CHECK-BE: lui  $1, 65535         # encoding: [0x3c,0x01,0xff,0xff]
# CHECK-BE: ori  $1, $1, 32767     # encoding: [0x34,0x21,0x7f,0xff]
# CHECK-BE: addu $1, $1, $9        # encoding: [0x00,0x29,0x08,0x21]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: lui  $1, 65535         # encoding: [0xff,0xff,0x01,0x3c]
# CHECK-LE: ori  $1, $1, 32767     # encoding: [0xff,0x7f,0x21,0x34]
# CHECK-LE: addu $1, $1, $9        # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

  ulhu $8, 32767($9)
# CHECK-BE: ori  $1, $zero, 32767  # encoding: [0x34,0x01,0x7f,0xff]
# CHECK-BE: addu $1, $1, $9        # encoding: [0x00,0x29,0x08,0x21]
# CHECK-BE: lbu  $8, 0($1)         # encoding: [0x90,0x28,0x00,0x00]
# CHECK-BE: lbu  $1, 1($1)         # encoding: [0x90,0x21,0x00,0x01]
# CHECK-BE: sll  $8, $8, 8         # encoding: [0x00,0x08,0x42,0x00]
# CHECK-BE: or   $8, $8, $1        # encoding: [0x01,0x01,0x40,0x25]
# CHECK-LE: ori  $1, $zero, 32767  # encoding: [0xff,0x7f,0x01,0x34]
# CHECK-LE: addu $1, $1, $9        # encoding: [0x21,0x08,0x29,0x00]
# CHECK-LE: lbu  $8, 1($1)         # encoding: [0x01,0x00,0x28,0x90]
# CHECK-LE: lbu  $1, 0($1)         # encoding: [0x00,0x00,0x21,0x90]
# CHECK-LE: sll  $8, $8, 8         # encoding: [0x00,0x42,0x08,0x00]
# CHECK-LE: or   $8, $8, $1        # encoding: [0x25,0x40,0x01,0x01]

1:
  add $4, $4, $4
