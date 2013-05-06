# RUN: llvm-mc -triple s390x-linux-gnu -show-encoding %s | FileCheck %s

#CHECK: oihf	%r0, 0                  # encoding: [0xc0,0x0c,0x00,0x00,0x00,0x00]
#CHECK: oihf	%r0, 4294967295         # encoding: [0xc0,0x0c,0xff,0xff,0xff,0xff]
#CHECK: oihf	%r15, 0                 # encoding: [0xc0,0xfc,0x00,0x00,0x00,0x00]

	oihf	%r0, 0
	oihf	%r0, 0xffffffff
	oihf	%r15, 0
