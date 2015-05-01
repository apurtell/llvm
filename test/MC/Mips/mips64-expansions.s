# RUN: llvm-mc %s -triple=mips64el-unknown-linux -show-encoding -mcpu=mips64r2 | FileCheck %s
#
# Test the 'dli' and 'dla' 64-bit variants of 'li' and 'la'.

# Immediate is <= 32 bits.
  dli $5, 123
# CHECK:     ori   $5, $zero, 123   # encoding: [0x7b,0x00,0x05,0x34]

  dli $6, -2345
# CHECK:     addiu $6, $zero, -2345 # encoding: [0xd7,0xf6,0x06,0x24]

  dli $7, 65538
# CHECK:     lui   $7, 1            # encoding: [0x01,0x00,0x07,0x3c]
# CHECK:     ori   $7, $7, 2        # encoding: [0x02,0x00,0xe7,0x34]

  dli $8, ~7
# CHECK:     addiu $8, $zero, -8    # encoding: [0xf8,0xff,0x08,0x24]

  dli $9, 0x10000
# CHECK:     lui   $9, 1            # encoding: [0x01,0x00,0x09,0x3c]
# CHECK-NOT: ori   $9, $9, 0        # encoding: [0x00,0x00,0x29,0x35]


# Positive immediate which is => 32 bits and <= 48 bits.
  dli $8, 0x100000000
# CHECK: lui  $8, 1                 # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: dsll $8, $8, 16            # encoding: [0x38,0x44,0x08,0x00]

  dli $8, 0x100000001
# CHECK: lui  $8, 1                 # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: dsll $8, $8, 16            # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori  $8, $8, 1             # encoding: [0x01,0x00,0x08,0x35]

  dli $8, 0x100010000
# CHECK: lui  $8, 1                 # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: ori  $8, $8, 1             # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll $8, $8, 16            # encoding: [0x38,0x44,0x08,0x00]

  dli $8, 0x100010001
# CHECK: lui  $8, 1                 # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: ori  $8, $8, 1             # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll $8, $8, 16            # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori  $8, $8, 1             # encoding: [0x01,0x00,0x08,0x35]


# Positive immediate which is > 48 bits.
  dli $8, 0x1000000000000
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]

  dli $8, 0x1000000000001
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]

  dli $8, 0x1000000010000
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]

  dli $8, 0x1000100000000
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]

  dli $8, 0x1000000010001
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]

  dli $8, 0x1000100010000
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]

  dli $8, 0x1000100000001
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]

  dli $8, 0x1000100010001
# CHECK: lui    $8, 1               # encoding: [0x01,0x00,0x08,0x3c]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 1           # encoding: [0x01,0x00,0x08,0x35]


# Negative immediate which is => 32 bits and <= 48 bits.
  dli $8, -0x100000000
# CHECK: lui    $8, 65535           # encoding: [0xff,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]

  dli $8, -0x100000001
# CHECK: lui    $8, 65535           # encoding: [0xff,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]

  dli $8, -0x100010000
# CHECK: lui    $8, 65535           # encoding: [0xff,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]

  dli $8, -0x100010001
# CHECK: lui    $8, 65535           # encoding: [0xff,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]


# Negative immediate which is > 48 bits.
  dli $8, -0x1000000000000
# CHECK: lui    $8, 65535           # encoding: [0xff,0xff,0x08,0x3c]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]

  dli $8, -0x1000000000001
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]

  dli $8, -0x1000000010000
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]

  dli $8, -0x1000100000000
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll32 $8, $8, 0           # encoding: [0x3c,0x40,0x08,0x00]

  dli $8, -0x1000000010001
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]

  dli $8, -0x1000100010000
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]

  dli $8, -0x1000100000001
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]

  dli $8, -0x1000100010001
# CHECK: lui    $8, 65534           # encoding: [0xfe,0xff,0x08,0x3c]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65534       # encoding: [0xfe,0xff,0x08,0x35]
# CHECK: dsll   $8, $8, 16          # encoding: [0x38,0x44,0x08,0x00]
# CHECK: ori    $8, $8, 65535       # encoding: [0xff,0xff,0x08,0x35]
