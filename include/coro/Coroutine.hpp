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

extern "C" {
void __cdecl coroSwapContext(coro::Coroutine* from, coro::Coroutine* to);
void __cdecl coroStart();
}

namespace coro {

class Stack {
public:
    Stack(uint64_t size);
    ~Stack(); 
    uint8_t* end() { return data_+sizeof(size_); }
    uint8_t* begin() { return data_; }
private:
    uint8_t* data_;
    uint64_t size_;
    friend class Coroutine;
};

class ExitException {
};

enum class CoroutineStatus { NEW, RUNNING, SUSPENDED, DEAD };

class Coroutine : public std::enable_shared_from_this<Coroutine> {
// A coroutine, or lightweight cooperative thread.  A coroutine runs a function
// that is allowed to suspend and resume at any point during its execution.
public:
    template <typename F>
    Coroutine(F func) : stack_(CORO_STACK_SIZE) { init(func); }
    void swap(); // Passes control to this coroutine

    static Ptr<Coroutine> current();
    static Ptr<Coroutine> hub();
private:
    Coroutine(); // Special constructor for the main thread.
    void init(std::function<void()> const& func);
    void commit(uint64_t addr);
    void exit();
    void start();

    uint8_t* stackPointer_; // This field must be the first field in the coroutine
    std::function<void()> func_; 
    CoroutineStatus status_;
    Stack stack_;

    friend void ::coroStart();
};

}
