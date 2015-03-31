# For zEC12 and above.
# RUN: llvm-mc -triple s390x-linux-gnu -mcpu=zEC12 -show-encoding %s | FileCheck %s

#CHECK: risbgn	%r0, %r0, 0, 0, 0       # encoding: [0xec,0x00,0x00,0x00,0x00,0x59]
#CHECK: risbgn	%r0, %r0, 0, 0, 63      # encoding: [0xec,0x00,0x00,0x00,0x3f,0x59]
#CHECK: risbgn	%r0, %r0, 0, 255, 0     # encoding: [0xec,0x00,0x00,0xff,0x00,0x59]
#CHECK: risbgn	%r0, %r0, 255, 0, 0     # encoding: [0xec,0x00,0xff,0x00,0x00,0x59]
#CHECK: risbgn	%r0, %r15, 0, 0, 0      # encoding: [0xec,0x0f,0x00,0x00,0x00,0x59]
#CHECK: risbgn	%r15, %r0, 0, 0, 0      # encoding: [0xec,0xf0,0x00,0x00,0x00,0x59]
#CHECK: risbgn	%r4, %r5, 6, 7, 8       # encoding: [0xec,0x45,0x06,0x07,0x08,0x59]

	risbgn	%r0,%r0,0,0,0
	risbgn	%r0,%r0,0,0,63
	risbgn	%r0,%r0,0,255,0
	risbgn	%r0,%r0,255,0,0
	risbgn	%r0,%r15,0,0,0
	risbgn	%r15,%r0,0,0,0
	risbgn	%r4,%r5,6,7,8

