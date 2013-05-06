# RUN: not llvm-mc -triple s390x-linux-gnu < %s 2> %t
# RUN: FileCheck < %t %s

#CHECK: error: invalid operand
#CHECK: clfhsi	-1, 0
#CHECK: error: invalid operand
#CHECK: clfhsi	4096, 0
#CHECK: error: invalid use of indexed addressing
#CHECK: clfhsi	0(%r1,%r2), 0
#CHECK: error: invalid operand
#CHECK: clfhsi	0, -1
#CHECK: error: invalid operand
#CHECK: clfhsi	0, 65536

	clfhsi	-1, 0
	clfhsi	4096, 0
	clfhsi	0(%r1,%r2), 0
	clfhsi	0, -1
	clfhsi	0, 65536
