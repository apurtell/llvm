; Make sure we don't get an assertion failure, even though this is a parse 
; error
; RUN: llvm-upgrade 2>&1 < %s > /dev/null | grep 'No arguments passed to a '

%ty = type void (int)

declare %ty* %foo()

void %test() {
        call %ty* %foo( )               ; <%ty*>:0 [#uses=0]
        ret void
}

