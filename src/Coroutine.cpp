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
#include "coro/Hub.hpp"
#include "coro/Error.hpp"

extern "C" {
coro::Coroutine* coroCurrent = coro::main().get();
}

void coroStart() throw() { coroCurrent->start(); }

#ifdef _WIN32
#include "Coroutine.win.inl"
#else
#include "Coroutine.unix.inl"
#endif


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
    data_ = (uint8_t*)malloc(size);
    assert(data_);
}

Stack::~Stack() {
// Throw exception
    if (data_) {
        free(data_);
    }
}

Coroutine::Coroutine() : stack_(0) {
// Constructor for the main coroutine.
    status_ = Coroutine::RUNNING;
    stackPointer_ = 0;
}

Coroutine::~Coroutine() {
// Destroy the coroutine & clean up its stack
    if (!stack_.begin()) {
        // This is the main coroutine; don't free up anything, because we did
        // not allocate a stack for it.
    } else if (status_ != Coroutine::DEAD && status_ != Coroutine::NEW) {
        assert(status_ != Coroutine::BLOCKED);
        status_ = Coroutine::DELETED;
        swap(); // Allow the coroutine to clean up its stack
    }
}

void Coroutine::init(std::function<void()> const& func) {
// Creates a new coroutine and allocates a stack for it.
    coro::main();
    func_ = func;
    status_ = Coroutine::NEW;

#ifdef _WIN32
    assert((((uint8_t*)this)+8)==(uint8_t*)&stackPointer_);
#else
    assert((((uint8_t*)this)+16)==(uint8_t*)&stackPointer_);
#endif

#ifdef _WIN32
    struct StackFrame {
        void* fs8;
        void* fs4;
        void* fs0;
        void* rdi;
        void* rsi;
        void* rdx;
        void* rcx;
        void* rbx;
        void* rax;
        void* rbp;
        void* returnAddr; // coroStart() stack frame here
    };
#else
    struct StackFrame {
        void* r15;
        void* r14;
        void* r13;
        void* r12;
        void* r11;
        void* r10;
        void* r9;
        void* r8;
        void* rdi;
        void* rsi;
        void* rdx;
        void* rcx;
        void* rbx;
        void* rax;
        void* rbp;
        void* returnAddr;
        void* padding;
    }; 
#endif

    StackFrame frame;
    memset(&frame, 0, sizeof(frame));
#ifdef _WIN32
    frame.fs0 = (void*)0xffffffff; // Root-level SEH handler
    frame.fs4 = stack_.end();// Top of stack
    frame.fs8 = stack_.begin(); // Bottom of stack
    // See: http://stackoverflow.com/questions/9249576/seh-setup-for-fibers-with-exception-chain-validation-sehop-active
#endif
    frame.returnAddr = (void*)coroStart;

    stackPointer_ = stack_.end();
    stackPointer_ -= sizeof(frame);
    memcpy(stackPointer_, &frame, sizeof(frame));
}

void Coroutine::yield() {
// Yield temporarily to the hub.  The hub will always reschedule a suspended
// coroutine to run.
    assert(coroCurrent == this);
    switch (coroCurrent->status_) {
    case Coroutine::RUNNING: coroCurrent->status_ = Coroutine::RUNNABLE; break;
    case Coroutine::DEAD: break;
    case Coroutine::DELETED: break; // fallthrough
    case Coroutine::RUNNABLE: // fallthrough
    case Coroutine::BLOCKED: // fallthrough
    case Coroutine::NEW: // fallthrough
    default: assert(!"illegal state"); break;
    }
    main()->swap();
}

void Coroutine::block() {
// Block the current coroutine until some I/O event occurs.  The coroutine will
// not be rescheduled until explicitly scheduled.

    // Anchor the coroutine, so that it doesn't get GC'ed while blocked on I/O.
    Ptr<Coroutine> anchor = shared_from_this();
    assert(coroCurrent == this);
    switch (status_) {
    case Coroutine::RUNNING: status_ = Coroutine::BLOCKED; break;
    case Coroutine::DEAD: break;
    case Coroutine::DELETED: break; // fallthrough
    case Coroutine::RUNNABLE: // fallthrough
    case Coroutine::BLOCKED: // fallthrough
    case Coroutine::NEW: // fallthrough
    default: assert(!"illegal state"); break;
    }
    hub()->blocked_++;
    main()->swap();
}

void Coroutine::unblock() {
// Unblock the coroutine when an event occurs.
    switch (status_) {
    case Coroutine::BLOCKED: status_ = Coroutine::RUNNABLE; break;
    case Coroutine::RUNNING: // fallthrough
    case Coroutine::DEAD: // fallthrough
    case Coroutine::DELETED: // fallthrough
    case Coroutine::RUNNABLE: // fallthrough
    case Coroutine::NEW: // fallthrough
    default: assert(!"illegal state"); break;
    }
    hub()->blocked_--;
    hub()->runnable_.push_back(shared_from_this());
}

void Coroutine::wait() {
// Block the current coroutine until some event occurs.  The coroutine will not
// be rescheduled until explicitly scheduled.

    // Anchor the coroutine, so that it doesn't get GC'ed while waiting.
    // FixMe: Should this be the case?  Maybe a waiting coroutine should be
    // collected.
    Ptr<Coroutine> anchor = shared_from_this();
    assert(coroCurrent == this);
    switch (status_) {
    case Coroutine::RUNNING: status_ = Coroutine::WAITING; break;
    case Coroutine::DEAD: break;
    case Coroutine::DELETED: break; // fallthrough
    case Coroutine::RUNNABLE: // fallthrough
    case Coroutine::BLOCKED: // fallthrough
    case Coroutine::NEW: // fallthrough
    default: assert(!"illegal state"); break;
    }
    hub()->waiting_++;
    main()->swap();
}

void Coroutine::notify() {
// Unblock the coroutine when an event occurs.
    switch (status_) {
    case Coroutine::WAITING: status_ = Coroutine::RUNNABLE; break;
    case Coroutine::RUNNABLE: return;
    case Coroutine::RUNNING: // fallthrough
    case Coroutine::DEAD: // fallthrough
    case Coroutine::DELETED: // fallthrough
    case Coroutine::NEW: // fallthrough
    default: assert(!"illegal state"); break;
    }
    hub()->waiting_--;
    hub()->runnable_.push_back(shared_from_this());
}

void Coroutine::swap() {
// Swaps control to this coroutine, causing the current coroutine to suspend.
// This function returns when another coroutine swaps back to this coroutine.
// If a coroutine dies it is asleep, then it throws an ExitException, which
// will cause the coroutine to exit.
    Coroutine* current = coroCurrent;
    switch (status_) {
    case Coroutine::DELETED: break;
    case Coroutine::RUNNABLE: status_ = Coroutine::RUNNING; break;
    case Coroutine::NEW: status_ = Coroutine::RUNNING; break;
    case Coroutine::BLOCKED: status_ = Coroutine::RUNNING; break;
    case Coroutine::RUNNING: return; // already running
    case Coroutine::DEAD: assert(!"coroutine is dead"); break;
    default: assert(!"illegal state"); break;
    }
    coroCurrent = this;
    coroSwapContext(current, this);
    switch (coroCurrent->status_) {
    case Coroutine::DELETED: if (!coroCurrent->isMain()) { throw ExitException(); } break;
    case Coroutine::RUNNING: break; 
    case Coroutine::RUNNABLE: // fallthrough
    case Coroutine::BLOCKED: // fallthrough
    case Coroutine::NEW: // fallthrough
    case Coroutine::DEAD: // fallthrough
    default: assert(!"illegal state"); break;
    }
}

void Coroutine::start() throw() {
// This function runs the coroutine from the given entry point.
    try {
        func_();
        exit(); // Fell off the end of the coroutine function
    } catch(ExitException const& ex) {
        exit(); // Coroutine was deallocated before exiting
    } catch(...) {
        assert(!"error: coroutine killed by exception");
    }
    assert(!"error: unreachable");
}

void Coroutine::exit() {
// This function runs when the coroutine "falls of the stack," that is, when it finishes executing.
    assert(coroCurrent == this);
    switch (status_) {
    case Coroutine::DELETED: break;
    case Coroutine::RUNNING: status_ = Coroutine::DEAD; break;
    case Coroutine::DEAD: // fallthrough
    case Coroutine::RUNNABLE: // fallthrough
    case Coroutine::NEW: // fallthrough
    default: assert(!"illegal state"); break;
    }
    main()->swap();
    assert(!"error: coroutine is dead");
}

Ptr<Coroutine> current() {
// Returns the coroutine that is currently executing.
    return coroCurrent->shared_from_this();
}

Ptr<Coroutine> main() {
// Returns the "main" coroutine (i.e., the main coroutine)
    static Ptr<Coroutine> main;
    if (!main) {
        main.reset(new Coroutine);
        registerSignalHandlers();
    }
    return main;
}

void yield() {
    current()->yield();
}

void sleep(Time const& time) {
    hub()->timeoutIs(Timeout(time, current()));
    current()->wait();
}


}
