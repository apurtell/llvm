# RUN: llvm-mc -triple s390x-linux-gnu -show-encoding %s | FileCheck %s

#CHECK: dlg	%r0, -524288            # encoding: [0xe3,0x00,0x00,0x00,0x80,0x87]
#CHECK: dlg	%r0, -1                 # encoding: [0xe3,0x00,0x0f,0xff,0xff,0x87]
#CHECK: dlg	%r0, 0                  # encoding: [0xe3,0x00,0x00,0x00,0x00,0x87]
#CHECK: dlg	%r0, 1                  # encoding: [0xe3,0x00,0x00,0x01,0x00,0x87]
#CHECK: dlg	%r0, 524287             # encoding: [0xe3,0x00,0x0f,0xff,0x7f,0x87]
#CHECK: dlg	%r0, 0(%r1)             # encoding: [0xe3,0x00,0x10,0x00,0x00,0x87]
#CHECK: dlg	%r0, 0(%r15)            # encoding: [0xe3,0x00,0xf0,0x00,0x00,0x87]
#CHECK: dlg	%r0, 524287(%r1,%r15)   # encoding: [0xe3,0x01,0xff,0xff,0x7f,0x87]
#CHECK: dlg	%r0, 524287(%r15,%r1)   # encoding: [0xe3,0x0f,0x1f,0xff,0x7f,0x87]
#CHECK: dlg	%r14, 0                 # encoding: [0xe3,0xe0,0x00,0x00,0x00,0x87]

	dlg	%r0, -524288
	dlg	%r0, -1
	dlg	%r0, 0
	dlg	%r0, 1
	dlg	%r0, 524287
	dlg	%r0, 0(%r1)
	dlg	%r0, 0(%r15)
	dlg	%r0, 524287(%r1,%r15)
	dlg	%r0, 524287(%r15,%r1)
	dlg	%r14, 0
