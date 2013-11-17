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

#pragma once

#include "coro/Common.hpp"
#include "coro/Time.hpp"

extern "C" {
void __cdecl coroSwapContext(coro::Coroutine* from, coro::Coroutine* to);
void __cdecl coroStart() throw();
}

namespace coro {

Ptr<Coroutine> current();
Ptr<Coroutine> main();
void yield();
void sleep(Time const& time);
void fault(int signo, siginfo_t* info, void* context);


class Stack {
// Lazily-allocated stack for a coroutine.  When a new coroutine stack is
// created, a block of virtual memory is reserved for the coroutine with memory
// protection enabled.  When the process first accesses a previously
// unallocated for the coroutine, the page is allocated and the permissions set
// to allow reads/writes.  In this way, the coroutine only uses the stack
// memory it needs.  A coroutine's stack never shrinks; to garbage-collect the
// stack, the coroutine must be destroyed.
public:
    Stack(uint64_t size);
    ~Stack(); 
    uint8_t* end() { return data_+size_; }
    uint8_t* begin() { return data_; }
private:
    uint8_t* data_;
    uint64_t size_;
    friend class Coroutine;
};

class ExitException {
// Thrown when a coroutine is destroyed, but the coroutine hasn't exited yet.
// The ExitException unwinds the coroutine's stack, so that destructors for all
// stack objects are called.  One should rarely (if ever) catch an
// ExitException, and if an ExitException is caught, it should be thrown again
// immediately.  If a coroutine attempts to yield after an ExitException has
// been thrown, then another ExitException is thrown from swap().
};


class Coroutine : public std::enable_shared_from_this<Coroutine> {
// A coroutine, or lightweight cooperative thread.  A coroutine runs a function
// that is allowed to suspend and resume at any point during its execution.
public:
    enum Status { NEW, RUNNING, SUSPENDED, BLOCKED, DEAD, DELETED };

    ~Coroutine();

    template <typename F>
    Coroutine(F func) : stack_(CORO_STACK_SIZE) { init(func); }
    Status status() const { return status_; }

private:
    Coroutine(); // Special constructor for the main thread.
    void init(std::function<void()> const& func);
    void commit(uint64_t addr);
    void exit();
    void start() throw();
    void swap(); // Passes control to this coroutine
    void yield(); 
    void block(); 
    bool isMain() { return !stack_.begin(); }

    uint8_t* stackPointer_; // This field must be the first field in the coroutine
    std::function<void()> func_; 
    Status status_;
    Stack stack_;

    friend Ptr<Coroutine> coro::current();
    friend Ptr<Coroutine> coro::main();
    friend void coro::yield();
    friend void coro::sleep(Time const& time);
    friend void ::coroStart() throw();
    friend void coro::fault(int signo, siginfo_t* info, void* context);
    friend class coro::Hub;
    friend class coro::Socket;
};

}
