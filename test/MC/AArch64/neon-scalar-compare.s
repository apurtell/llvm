// RUN: llvm-mc -triple aarch64-none-linux-gnu -mattr=+neon -show-encoding < %s | FileCheck %s

// Check that the assembler can handle the documented syntax for AArch64

//----------------------------------------------------------------------
// Scalar Compare Bitwise Equal
//----------------------------------------------------------------------

         cmeq d20, d21, d22

// CHECK: cmeq d20, d21, d22   // encoding: [0xb4,0x8e,0xf6,0x7e]

//----------------------------------------------------------------------
// Scalar Compare Bitwise Equal To Zero
//----------------------------------------------------------------------

         cmeq d20, d21, #0x0

// CHECK: cmeq d20, d21, #{{0x0|0}}   // encoding: [0xb4,0x9a,0xe0,0x5e]

//----------------------------------------------------------------------
// Scalar Compare Unsigned Higher Or Same
//----------------------------------------------------------------------

         cmhs d20, d21, d22

// CHECK: cmhs d20, d21, d22   // encoding: [0xb4,0x3e,0xf6,0x7e]

//----------------------------------------------------------------------
// Scalar Compare Signed Greather Than Or Equal
//----------------------------------------------------------------------

         cmge d20, d21, d22

// CHECK: cmge d20, d21, d22    // encoding: [0xb4,0x3e,0xf6,0x5e]

//----------------------------------------------------------------------
// Scalar Compare Signed Greather Than Or Equal To Zero
//----------------------------------------------------------------------

         cmge d20, d21, #0x0

// CHECK: cmge d20, d21, #{{0x0|0}}   // encoding: [0xb4,0x8a,0xe0,0x7e]

//----------------------------------------------------------------------
// Scalar Compare Unsigned Higher
//----------------------------------------------------------------------

         cmhi d20, d21, d22

// CHECK: cmhi d20, d21, d22   // encoding: [0xb4,0x36,0xf6,0x7e]
//----------------------------------------------------------------------
// Scalar Compare Signed Greater Than
//----------------------------------------------------------------------

         cmgt d20, d21, d22

// CHECK: cmgt d20, d21, d22   // encoding: [0xb4,0x36,0xf6,0x5e]

//----------------------------------------------------------------------
// Scalar Compare Signed Greater Than Zero
//----------------------------------------------------------------------

         cmgt d20, d21, #0x0

// CHECK: cmgt d20, d21, #{{0x0|0}}   // encoding: [0xb4,0x8a,0xe0,0x5e]

//----------------------------------------------------------------------
// Scalar Compare Signed Less Than Or Equal To Zero
//----------------------------------------------------------------------

         cmle d20, d21, #0x0

// CHECK: cmle d20, d21, #{{0x0|0}}   // encoding: [0xb4,0x9a,0xe0,0x7e]

//----------------------------------------------------------------------
// Scalar Compare Less Than Zero
//----------------------------------------------------------------------

         cmlt d20, d21, #0x0

// CHECK: cmlt d20, d21, #{{0x0|0}}   // encoding: [0xb4,0xaa,0xe0,0x5e]

//----------------------------------------------------------------------
// Scalar Compare Bitwise Test Bits
//----------------------------------------------------------------------

         cmtst d20, d21, d22

// CHECK: cmtst d20, d21, d22   // encoding: [0xb4,0x8e,0xf6,0x5e]
