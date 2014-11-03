; RUN: llc -verify-machineinstrs -mtriple=aarch64-none-linux-gnu %s -filetype=obj -o %t
; RUN: llvm-objdump -s %t | FileCheck %s --check-prefix=CHECK
; RUN: llvm-readobj -r %t | FileCheck %s --check-prefix=RELOC

declare i32 @__gxx_personality_v0(...)

declare void @bar()

define i64 @foo(i64 %lhs, i64 %rhs) {
  invoke void @bar() to label %end unwind label %clean
end:
 ret i64 0

clean:
  %tst = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) cleanup
  ret i64 42
}

; CHECK: Contents of section .eh_frame:
; CHECK: 0000 1c000000 00000000 037a504c 5200017c  .........zPLR..|
; CHECK: 0010 1e0b0000 00000000 00000000 1b0c1f00  ................

; Don't really care about the rest:

; 0020 1c000000 24000000 00000000 24000000  ....$.......$...
; 0030 08000000 00000000 00440c1f 10449e02  .........D...D..

; The key test here is that the personality routine is sanely encoded (under the
; small memory model it must be an 8-byte value for full generality: code+data <
; 4GB, but you might need both +4GB and -4GB depending on where things end
; up. However, for completeness:

; First CIE:
; ----------
; 1c000000: Length = 0x1c
; 00000000: This is a CIE
; 03: Version 3
; 7a 50 4c 52 00: Augmentation string "zPLR" (personality routine, language-specific data, pointer format)
; 01: Code alignment factor 1
; 78: Data alignment factor: -8
; 1e: Return address in x30
; 07: Augmentation data 0xb bytes (this is key!)
; 00: Personality encoding is DW_EH_PE_absptr
; 00 00 00 00 00 00 00 00: First part of aug (personality routine). Relocated, obviously
; 00: Second part of aug (language-specific data): absolute pointer format used
; 1b: pointer format: pc-relative signed 4-byte. Just like GNU.
; 0c 1f 00: Initial instructions ("DW_CFA_def_cfa x31 ofs 0" in this case)

; Check the relocations applied to the .eh_frame section.
; These must not contain section-relative relocations to a section which
; is part of a group, as it could be removed.
; RELOC: Section ({{[0-9]+}}) .rela.eh_frame {
; RELOC-NEXT:   0x{{[0-9A-F]+}} R_AARCH64_ABS64 __gxx_personality_v0 0x0
; RELOC-NEXT:   0x{{[0-9A-F]+}} R_AARCH64_PREL32 foo 0x0
; RELOC-NEXT:   0x{{[0-9A-F]+}} R_AARCH64_ABS64 .gcc_except_table 0x0
; RELOC-NEXT: }
