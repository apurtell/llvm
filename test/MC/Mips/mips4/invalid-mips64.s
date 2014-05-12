# Instructions that are invalid
#
# FIXME: This test should be moved to the mips5 directory when mips5 is supported
# RUN: not llvm-mc %s -triple=mips64-unknown-linux -show-encoding -mcpu=mips4 \
# RUN:     2>%t1
# RUN: FileCheck %s < %t1

        .set noat
	clo	$t3,$a1       # CHECK: :[[@LINE]]:{{[0-9]+}}: error: instruction requires a CPU feature not currently enabled
	clz	$sp,$gp       # CHECK: :[[@LINE]]:{{[0-9]+}}: error: instruction requires a CPU feature not currently enabled
	dclo	$s2,$a2       # CHECK: :[[@LINE]]:{{[0-9]+}}: error: instruction requires a CPU feature not currently enabled
	dclz	$s0,$t9       # CHECK: :[[@LINE]]:{{[0-9]+}}: error: instruction requires a CPU feature not currently enabled
	luxc1	$f19,$s6($s5) # CHECK: :[[@LINE]]:{{[0-9]+}}: error: instruction requires a CPU feature not currently enabled
	suxc1	$f12,$k1($t5) # CHECK: :[[@LINE]]:{{[0-9]+}}: error: instruction requires a CPU feature not currently enabled
