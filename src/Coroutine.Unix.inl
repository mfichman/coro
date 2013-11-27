/*
 * Copyright (c) 2013 Matt Fichman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, APEXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

namespace coro {

/*
static struct sigaction sigsegv;
static struct sigaction sigbus;
static char signalstack[SIGSTKSZ];

void fault(int signo, siginfo_t* info, void* context) {
    if (signo != SIGBUS && signo != SIGSEGV) {
        abort();
    }
    uint64_t start = (uint64_t)coroCurrent->stack_.begin();
    uint64_t end = (uint64_t)coroCurrent->stack_.end();
    uint64_t addr = (uint64_t)info->si_addr;
    if (addr < start || addr >= end) {
        if (signo == SIGBUS) {
            struct sigaction self;
            sigaction(SIGBUS, &sigbus, &self);
            raise(SIGBUS);
        } else if (signo == SIGSEGV) {
            struct sigaction self;
            sigaction(SIGSEGV, &sigsegv, &self);
            raise(SIGSEGV);
        }
    } else {
        coroCurrent->commit(addr);
    }
}
*/

void registerSignalHandlers() {
    // Set up the signal handlers for SIGBUS/SIGSEGV to catch stack protection
    // errors and grow the stack when it runs out of space.
/*
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigfillset(&sa.sa_mask);
    sa.sa_sigaction = ::coro::fault;
    sa.sa_flags = SA_SIGINFO|SA_ONSTACK;
    sigaction(SIGSEGV, &sa, &sigsegv);
    sigaction(SIGBUS, &sa, &sigbus); 
  
    stack_t stack;
    stack.ss_sp = signalstack;
    stack.ss_flags = 0;
    stack.ss_size = sizeof(signalstack);
    sigaltstack(&stack, 0);
*/
}

}
