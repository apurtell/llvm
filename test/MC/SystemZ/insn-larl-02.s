# RUN: not llvm-mc -triple s390x-linux-gnu < %s 2> %t
# RUN: FileCheck < %t %s

#CHECK: error: offset out of range
#CHECK: larl	%r0, -0x1000000002
#CHECK: error: offset out of range
#CHECK: larl	%r0, -1
#CHECK: error: offset out of range
#CHECK: larl	%r0, 1
#CHECK: error: offset out of range
#CHECK: larl	%r0, 0x100000000

	larl	%r0, -0x1000000002
	larl	%r0, -1
	larl	%r0, 1
	larl	%r0, 0x100000000
