; This test ensures that "strength reduction" of conditional expressions are
; working.  Basically this boils down to converting setlt,gt,le,ge instructions
; into equivalent setne,eq instructions.
;

; RUN: if as < %s | opt -instcombine | dis | grep -v seteq | grep -v setne | grep set
; RUN: then exit 1
; RUN: else exit 0
; RUN: fi

bool "test1"(uint %A) {
	%B = setge uint %A, 1   ; setne %A, 0
	ret bool %B
}

bool "test2"(uint %A) {
	%B = setgt uint %A, 0   ; setne %A, 0
	ret bool %B
}

bool "test3"(sbyte %A) {
	%B = setge sbyte %A, -127   ; setne %A, -128
	ret bool %B
}
