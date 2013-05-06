# RUN: llvm-mc -triple s390x-linux-gnu -show-encoding %s | FileCheck %s

#CHECK: meeb	%f0, 0                  # encoding: [0xed,0x00,0x00,0x00,0x00,0x17]
#CHECK: meeb	%f0, 4095               # encoding: [0xed,0x00,0x0f,0xff,0x00,0x17]
#CHECK: meeb	%f0, 0(%r1)             # encoding: [0xed,0x00,0x10,0x00,0x00,0x17]
#CHECK: meeb	%f0, 0(%r15)            # encoding: [0xed,0x00,0xf0,0x00,0x00,0x17]
#CHECK: meeb	%f0, 4095(%r1,%r15)     # encoding: [0xed,0x01,0xff,0xff,0x00,0x17]
#CHECK: meeb	%f0, 4095(%r15,%r1)     # encoding: [0xed,0x0f,0x1f,0xff,0x00,0x17]
#CHECK: meeb	%f15, 0                 # encoding: [0xed,0xf0,0x00,0x00,0x00,0x17]

	meeb	%f0, 0
	meeb	%f0, 4095
	meeb	%f0, 0(%r1)
	meeb	%f0, 0(%r15)
	meeb	%f0, 4095(%r1,%r15)
	meeb	%f0, 4095(%r15,%r1)
	meeb	%f15, 0
