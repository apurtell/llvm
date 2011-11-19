; RUN: llc < %s -march=x86-64 -mattr=+ssse3,-avx | FileCheck %s -check-prefix=SSSE3
; RUN: llc < %s -march=x86-64 -mattr=-ssse3,+avx | FileCheck %s -check-prefix=AVX

; SSSE3: phaddw1:
; SSSE3-NOT: vphaddw
; SSSE3: phaddw
; AVX: phaddw1:
; AVX: vphaddw
define <8 x i16> @phaddw1(<8 x i16> %x, <8 x i16> %y) {
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
  %b = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  %r = add <8 x i16> %a, %b
  ret <8 x i16> %r
}

; SSSE3: phaddw2:
; SSSE3-NOT: vphaddw
; SSSE3: phaddw
; AVX: phaddw2:
; AVX: vphaddw
define <8 x i16> @phaddw2(<8 x i16> %x, <8 x i16> %y) {
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 2, i32 5, i32 6, i32 9, i32 10, i32 13, i32 14>
  %b = shufflevector <8 x i16> %y, <8 x i16> %x, <8 x i32> <i32 8, i32 11, i32 12, i32 15, i32 0, i32 3, i32 4, i32 7>
  %r = add <8 x i16> %a, %b
  ret <8 x i16> %r
}

; SSSE3: phaddd1:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd1:
; AVX: vphaddd
define <4 x i32> @phaddd1(<4 x i32> %x, <4 x i32> %y) {
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phaddd2:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd2:
; AVX: vphaddd
define <4 x i32> @phaddd2(<4 x i32> %x, <4 x i32> %y) {
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 2, i32 5, i32 6>
  %b = shufflevector <4 x i32> %y, <4 x i32> %x, <4 x i32> <i32 4, i32 7, i32 0, i32 3>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phaddd3:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd3:
; AVX: vphaddd
define <4 x i32> @phaddd3(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 3, i32 5, i32 7>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phaddd4:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd4:
; AVX: vphaddd
define <4 x i32> @phaddd4(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 2, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 3, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phaddd5:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd5:
; AVX: vphaddd
define <4 x i32> @phaddd5(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 3, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 2, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phaddd6:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd6:
; AVX: vphaddd
define <4 x i32> @phaddd6(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 undef, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 undef, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phaddd7:
; SSSE3-NOT: vphaddd
; SSSE3: phaddd
; AVX: phaddd7:
; AVX: vphaddd
define <4 x i32> @phaddd7(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 3, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phsubw1:
; SSSE3-NOT: vphsubw
; SSSE3: phsubw
; AVX: phsubw1:
; AVX: vphsubw
define <8 x i16> @phsubw1(<8 x i16> %x, <8 x i16> %y) {
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
  %b = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  %r = sub <8 x i16> %a, %b
  ret <8 x i16> %r
}

; SSSE3: phsubd1:
; SSSE3-NOT: vphsubd
; SSSE3: phsubd
; AVX: phsubd1:
; AVX: vphsubd
define <4 x i32> @phsubd1(<4 x i32> %x, <4 x i32> %y) {
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phsubd2:
; SSSE3-NOT: vphsubd
; SSSE3: phsubd
; AVX: phsubd2:
; AVX: vphsubd
define <4 x i32> @phsubd2(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 3, i32 5, i32 7>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phsubd3:
; SSSE3-NOT: vphsubd
; SSSE3: phsubd
; AVX: phsubd3:
; AVX: vphsubd
define <4 x i32> @phsubd3(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 2, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 3, i32 undef, i32 undef>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

; SSSE3: phsubd4:
; SSSE3-NOT: vphsubd
; SSSE3: phsubd
; AVX: phsubd4:
; AVX: vphsubd
define <4 x i32> @phsubd4(<4 x i32> %x) {
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 undef, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 undef, i32 undef, i32 undef>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}
