# RUN: llvm-mc -triple s390x-linux-gnu -show-encoding %s | FileCheck %s

#CHECK: oilf	%r0, 0                  # encoding: [0xc0,0x0d,0x00,0x00,0x00,0x00]
#CHECK: oilf	%r0, 4294967295         # encoding: [0xc0,0x0d,0xff,0xff,0xff,0xff]
#CHECK: oilf	%r15, 0                 # encoding: [0xc0,0xfd,0x00,0x00,0x00,0x00]

	oilf	%r0, 0
	oilf	%r0, 0xffffffff
	oilf	%r15, 0
