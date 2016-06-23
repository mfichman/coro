# Copyright (c) 2010 Matt Fichman
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, APEXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# Pointer to the current coroutine.  Used by Coroutine__resume() to set the
# caller of the coroutine that is being resumed, and by Coroutine__yield() to
# switch from the current coroutine to the caller.


.section __TEXT,__text,regular,pure_instructions
.globl _coroSwapContext
.align 4, 0x90
_coroSwapContext: # (from, to)
    # Resume the coroutine passed in as the first argument by saving the state
    # of the current coroutine, and loading the other corountine's state.
    # Then, 'return' to the caller of the other coroutine's yield() invocation.
    #
    # **** NOTE: If any of the 'push' instructions below change, then
    # Coroutine.c must also be modified!!!
    #
    # On entry to this function, the stack looks like this:
    # rsi    to 
    # rdi    from
    pushq %rbp
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, 16(%rdi) # Save the sp for 'from'
    movq 16(%rsi), %rsp # Restore the sp for 'to'
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    popq %rbp
    ret
