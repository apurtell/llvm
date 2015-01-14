@ RUN: not llvm-mc -triple armv8 -mattr=-fp-armv8 -show-encoding < %s 2>&1 | FileCheck %s

vmaxnm.f32 s4, d5, q1
@ CHECK: error: invalid operand for instruction
vmaxnm.f64.f64 s4, d5, q1
@ CHECK: error: invalid operand for instruction
vmaxnmge.f64.f64 s4, d5, q1
@ CHECK: error: instruction 'vmaxnm' is not predicable, but condition code specified

vcvta.s32.f32 s1, s2
@ CHECK: error: instruction requires: FPARMv8
vcvtp.u32.f32 s1, d2
@ CHECK: error: invalid operand for instruction
vcvtp.f32.u32 d1, q2
@ CHECK: error: invalid operand for instruction
vcvtplo.f32.u32 s1, s2
@ CHECK: error: instruction 'vcvtp' is not predicable, but condition code specified

vrinta.f64.f64 s3, d12
@ CHECK: error: invalid operand for instruction
vrintn.f32 d3, q12
@ CHECK: error: invalid operand for instruction
vrintz.f32 d3, q12
@ CHECK: error: invalid operand for instruction
vrintmge.f32.f32 d3, d4
@ CHECK: error: instruction 'vrintm' is not predicable, but condition code specified

aesd.8  q0, s1
@ CHECK: error: invalid operand for instruction
aese.8  s0, q1
@ CHECK: error: invalid operand for instruction
aesimc.8  s0, q1
@ CHECK: error: invalid operand for instruction
aesmc.8  q0, d1
@ CHECK: error: invalid operand for instruction
aesdge.8 q0, q1
@ CHECK: error: instruction 'aesd' is not predicable, but condition code specified

sha1h.32  d0, q1
@ CHECK: error: invalid operand for instruction
sha1su1.32  q0, s1
@ CHECK: error: invalid operand for instruction
sha256su0.32  s0, q1
@ CHECK: error: invalid operand for instruction
sha1heq.32  q0, q1
@ CHECK: error: instruction 'sha1h' is not predicable, but condition code specified

sha1c.32  s0, d1, q2
@ CHECK: error: invalid operand for instruction
sha1m.32  q0, s1, q2
@ CHECK: error: invalid operand for instruction
sha1p.32  s0, q1, q2
@ CHECK: error: invalid operand for instruction
sha1su0.32  d0, q1, q2
@ CHECK: error: invalid operand for instruction
sha256h.32  q0, s1, q2
@ CHECK: error: invalid operand for instruction
sha256h2.32  q0, q1, s2
@ CHECK: error: invalid operand for instruction
sha256su1.32  s0, d1, q2
@ CHECK: error: invalid operand for instruction
sha256su1lt.32  q0, d1, q2
@ CHECK: error: instruction 'sha256su1' is not predicable, but condition code specified

vmull.p64 q0, s1, s3
@ CHECK: error: invalid operand for instruction
vmull.p64 s1, d2, d3
@ CHECK: error: invalid operand for instruction
vmullge.p64 q0, d16, d17
@ CHECK: error: instruction 'vmull' is not predicable, but condition code specified
