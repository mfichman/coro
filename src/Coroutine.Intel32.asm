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

.MODEL flat, C
.STACK 100h

.DATA
EXTERN coroCurrent:DWORD

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
    ; rsp+8  to
    ; rsp+4  from
    ; rsp+0  return address
    mov eax, [esp+8] ; Load 'to'
    mov ecx, [esp+4] ; Load 'from'
    mov [coroCurrent], eax
    ; Set the 'current coroutine' equal to ARG1

    push ebp
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    mov [ecx+8], esp ; Save the sp for 'from'
    mov esp, [eax+8] ; Restore the sp for 'to'
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret
coroSwapContext ENDP
END
