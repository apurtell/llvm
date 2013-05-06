# RUN: llvm-mc -triple s390x-linux-gnu -show-encoding %s | FileCheck %s

#CHECK: afi	%r0, -2147483648        # encoding: [0xc2,0x09,0x80,0x00,0x00,0x00]
#CHECK: afi	%r0, -1                 # encoding: [0xc2,0x09,0xff,0xff,0xff,0xff]
#CHECK: afi	%r0, 0                  # encoding: [0xc2,0x09,0x00,0x00,0x00,0x00]
#CHECK: afi	%r0, 1                  # encoding: [0xc2,0x09,0x00,0x00,0x00,0x01]
#CHECK: afi	%r0, 2147483647         # encoding: [0xc2,0x09,0x7f,0xff,0xff,0xff]
#CHECK: afi	%r15, 0                 # encoding: [0xc2,0xf9,0x00,0x00,0x00,0x00]

	afi	%r0, -1 << 31
	afi	%r0, -1
	afi	%r0, 0
	afi	%r0, 1
	afi	%r0, (1 << 31) - 1
	afi	%r15, 0
