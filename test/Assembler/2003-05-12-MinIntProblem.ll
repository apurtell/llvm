; RUN: llvm-as < %s | llvm-dis | grep -- -2147483648
; RUN: verify-uselistorder %s -preserve-bc-use-list-order -num-shuffles=5

define i32 @foo() {
        ret i32 -2147483648
}
