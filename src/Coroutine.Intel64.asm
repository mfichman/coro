; Copyright (c) 2010 Matt Fichman
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, APEXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.


; Pointer to the current coroutine.  Used by Coroutine__resume() to set the
; 'caller' of the coroutine that is being resumed, and by Coroutine__yield() to
; switch from the current coroutine to the caller.

;.MODEL flat, C
;.STACK 100h

.DATA
EXTERN coroCurrent:QWORD

.CODE
coroSwapContext PROC ; (from, to)
    ; Resume the coroutine passed in as the first argument by saving the state
    ; of the current coroutine, and loading the other corountine's state.
    ; Then, 'return' to the caller of the other coroutine's yield() invocation.
    ;
    ; **** NOTE: If any of the 'push' instructions below change, then
    ; Coroutine.c must also be modified!!!
    ;
    ; On entry to this function, the stack looks like this;
    ; rcx  to
    ; rdx  from
    ; rsp+0  return address
    mov [coroCurrent], rdx
    ; Set the 'current coroutine' equal to to

    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    push qword ptr gs:[0]
    push qword ptr gs:[8]
    push qword ptr gs:[16]
    ; Save structured exception handling chain

    mov [rcx+16], rsp ; Save the sp for 'from'
    mov rsp, [rdx+16] ; Restore the sp for 'to'

    pop qword ptr gs:[16]
    pop qword ptr gs:[8]
    pop qword ptr gs:[0]
    ; Restore structured exception handling chain

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    ret
coroSwapContext ENDP
END
