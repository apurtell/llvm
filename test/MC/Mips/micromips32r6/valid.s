# RUN: llvm-mc %s -triple=mips-unknown-linux -show-encoding -mcpu=mips32r6 -mattr=micromips | FileCheck %s

  .set noat
  add $3, $4, $5           # CHECK: add $3, $4, $5      # encoding: [0x00,0xa4,0x19,0x10]
  addiu $3, $4, 1234       # CHECK: addiu $3, $4, 1234  # encoding: [0x30,0x64,0x04,0xd2]
  addu $3, $4, $5          # CHECK: addu $3, $4, $5     # encoding: [0x00,0xa4,0x19,0x50]
  addiupc $4, 100          # CHECK: addiupc $4, 100     # encoding: [0x78,0x80,0x00,0x19]
  aluipc $3, 56            # CHECK: aluipc $3, 56       # encoding: [0x78,0x7f,0x00,0x38]
  and $3, $4, $5           # CHECK: and $3, $4, $5      # encoding: [0x00,0xa4,0x1a,0x50]
  andi $3, $4, 1234        # CHECK: andi $3, $4, 1234   # encoding: [0xd0,0x64,0x04,0xd2]
  auipc $3, -1             # CHECK: auipc $3, -1        # encoding: [0x78,0x7e,0xff,0xff]
  align $4, $2, $3, 2      # CHECK: align $4, $2, $3, 2 # encoding: [0x00,0x43,0x24,0x1f]
  aui $3,$2,-23            # CHECK: aui $3, $2, -23     # encoding: [0x10,0x62,0xff,0xe9]
  balc 14572256            # CHECK: balc 14572256       # encoding: [0xb4,0x37,0x96,0xb8]
  bc 14572256              # CHECK: bc 14572256         # encoding: [0x94,0x37,0x96,0xb8]
  bitswap $4, $2           # CHECK: bitswap $4, $2      # encoding: [0x00,0x44,0x0b,0x3c]
  cache 1, 8($5)           # CHECK: cache 1, 8($5)      # encoding: [0x20,0x25,0x60,0x08]
  clo $11, $a1             # CHECK: clo $11, $5         # encoding: [0x01,0x65,0x4b,0x3c]
  clz $sp, $gp             # CHECK: clz $sp, $gp        # encoding: [0x03,0x80,0xe8,0x50]
  div $3, $4, $5           # CHECK: div $3, $4, $5      # encoding: [0x00,0xa4,0x19,0x18]
  divu $3, $4, $5          # CHECK: divu $3, $4, $5     # encoding: [0x00,0xa4,0x19,0x98]
  jialc $5, 256            # CHECK: jialc $5, 256       # encoding: [0x80,0x05,0x01,0x00]
  jic   $5, 256            # CHECK: jic $5, 256         # encoding: [0xa0,0x05,0x01,0x00]
  lsa $2, $3, $4, 3        # CHECK: lsa  $2, $3, $4, 3  # encoding: [0x00,0x43,0x26,0x0f]
  lwpc    $2,268           # CHECK: lwpc $2, 268        # encoding: [0x78,0x48,0x00,0x43]
  mod $3, $4, $5           # CHECK: mod $3, $4, $5      # encoding: [0x00,0xa4,0x19,0x58]
  modu $3, $4, $5          # CHECK: modu $3, $4, $5     # encoding: [0x00,0xa4,0x19,0xd8]
  mul $3, $4, $5           # CHECK mul $3, $4, $5       # encoding: [0x00,0xa4,0x18,0x18]
  muh $3, $4, $5           # CHECK muh $3, $4, $5       # encoding: [0x00,0xa4,0x18,0x58]
  mulu $3, $4, $5          # CHECK mulu $3, $4, $5      # encoding: [0x00,0xa4,0x18,0x98]
  muhu $3, $4, $5          # CHECK muhu $3, $4, $5      # encoding: [0x00,0xa4,0x18,0xd8]
  nor $3, $4, $5           # CHECK: nor $3, $4, $5      # encoding: [0x00,0xa4,0x1a,0xd0]
  or $3, $4, $5            # CHECK: or $3, $4, $5       # encoding: [0x00,0xa4,0x1a,0x90]
  ori $3, $4, 1234         # CHECK: ori $3, $4, 1234    # encoding: [0x50,0x64,0x04,0xd2]
  pref 1, 8($5)            # CHECK: pref 1, 8($5)       # encoding: [0x60,0x25,0x20,0x08]
  seleqz $2,$3,$4          # CHECK: seleqz $2, $3, $4   # encoding: [0x00,0x83,0x11,0x40]
  selnez $2,$3,$4          # CHECK: selnez $2, $3, $4   # encoding: [0x00,0x83,0x11,0x80]
  sub $3, $4, $5           # CHECK: sub $3, $4, $5      # encoding: [0x00,0xa4,0x19,0x90]
  subu $3, $4, $5          # CHECK: subu $3, $4, $5     # encoding: [0x00,0xa4,0x19,0xd0]
  xor $3, $4, $5           # CHECK: xor $3, $4, $5      # encoding: [0x00,0xa4,0x1b,0x10]
  xori $3, $4, 1234        # CHECK: xori $3, $4, 1234   # encoding: [0x70,0x64,0x04,0xd2]

