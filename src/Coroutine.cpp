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

#include "coro/Common.hpp"
#include "coro/Coroutine.hpp"
#include <iostream>

extern "C" {
coro::Coroutine* coroCurrent = coro::Coroutine::hub().get();
}

void coroStart() { coroCurrent->start(); }

namespace coro {

uint64_t pageRound(uint64_t addr, uint64_t multiple) {
// Rounds 'base' to the nearest 'multiple'
    return (addr/multiple)*multiple;
}

uint64_t pageSize() {
// Returns the system page size.
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize*8; 
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

Stack::Stack(uint64_t size) : data_(0), size_(size) {
// Lazily initializes a large stack for the coroutine.  Initially, the
// coroutine stack is quite small, to lower memory usage.  As the stack grows,
// the Coroutine::fault handler will commit memory pages for the coroutine.
    if (size == 0) { return; }
#ifdef _WIN32
    data_ = (uint8_t*)VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    if (!data_) {
        throw std::bad_alloc();
    }
#else
#ifdef __linux__
    data_ = mmap(0, size, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
#else
    data_ = mmap(0, size, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
#endif
    if (data_ == MAP_FAILED) {
        throw std::bad_alloc();
    }
#endif
}

Stack::~Stack() {
    if (data_) {
        VirtualFree((LPVOID)data_, size_, MEM_RELEASE); 
    }
}

Coroutine::Coroutine() : stack_(0) {
// Constructor for the hub coroutine.
    status_ = CoroutineStatus::RUNNING;
    stackPointer_ = 0;
}

void Coroutine::init(std::function<void()> const& func) {
// Creates a new coroutine and allocates a stack for it.
    func_ = func;
    status_ = CoroutineStatus::NEW;
    commit((uint64_t)stack_.end()-1); 
    // Commit the page at the top of the coroutine stack

    assert((((uint8_t*)this)+8)==(uint8_t*)&stackPointer_);

    struct StackFrameIntel32 {
        void* returnAddr; // coroStart() stack frame here
        void* rbp;
        void* rax;
        void* rbx;
        void* rcx;
        void* rdx;
        void* rsi;
        void* rdi;
    };

    StackFrameIntel32 frame{};
    frame.returnAddr = coroStart;

    stackPointer_ = stack_.end();
    stackPointer_ -= sizeof(frame);
    memcpy(stackPointer_, &frame, sizeof(frame));
}

void Coroutine::swap() {
// Swaps control to this coroutine, causing the current coroutine to suspend.
// This function returns when another coroutine swaps back to this coroutine.
// If a coroutine dies it is asleep, then it throws an ExitException, which
// will cause the coroutine to exit.
    coroCurrent->status_ = CoroutineStatus::SUSPENDED;
    status_ = CoroutineStatus::RUNNING;
    coroSwapContext(coroCurrent, this);
    if (CoroutineStatus::DEAD == status_) {
        throw ExitException();
    }
    assert(coroCurrent == this);
    assert(status_ == CoroutineStatus::RUNNING);
}

Ptr<Coroutine> Coroutine::current() {
// Returns the coroutine that is currently executing.
    return coroCurrent->shared_from_this();
}

Ptr<Coroutine> Coroutine::hub() {
// Returns the "hub" coroutine (i.e., the main coroutine)
    static Ptr<Coroutine> hub(new Coroutine);
    return hub;
}

void Coroutine::start() {
// This function runs the coroutine from the given entry point.
    try {
        func_();
        exit(); // OK, fell of the end of the coroutine function
    } catch(ExitException const& ex) {
        exit(); // OK, coroutine was deallocated
    } catch(...) {
        assert(!"error: coroutine killed by exception");
    }
}

void Coroutine::exit() {
// This function runs when the coroutine "falls of the stack," that is, when it finishes executing.
    hub()->swap();
}

void Coroutine::commit(uint64_t addr) {
// Ensures that 'addr' is allocated, and that the next page in the stack is a
// guard page.
    uint64_t psize = pageSize();
    uint64_t page = pageRound(addr, psize);
    uint64_t len = (uint64_t)stack_.end()-page;
    // Allocate all pages between stack_min and the current page.
#ifdef _WIN32
    uint64_t glen = psize;
    uint64_t guard = page-glen;
    assert(page < (uint64_t)stack_.end());
    if (!VirtualAlloc((LPVOID)page, len, MEM_COMMIT, PAGE_READWRITE)) {
        abort();     
    }
    // Create a guard page right after the current page.
    if (!VirtualAlloc((LPVOID)guard, glen, MEM_COMMIT, PAGE_READWRITE|PAGE_GUARD)) {
        abort();     
    }
#else
    assert(page < (uint64_t)stack_.end());
    if (mprotect((void*)page, len, PROT_READ|PROT_WRITE) == -1) {
        abort();
    }
#endif


}

}
