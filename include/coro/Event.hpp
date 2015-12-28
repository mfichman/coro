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

namespace coro {

class EventRecord {
public:
    EventRecord(Ptr<Coroutine> coro);
    EventRecord();
    Ptr<Coroutine> coroutine() const { return coroutine_; }

private:
    Ptr<Coroutine> coroutine_;
};

typedef size_t EventWaitToken;

class Event {
// An event is a coroutine synchronization primitive.  A coroutine that waits
// on an event is put to sleep until another coroutine wakes it via the notify
// method.  By calling wait() inside a conditional loop, a coroutine can
// efficiently wait for a condition to become true -- thus enabling it to be
// used for triggers, etc.
public:
    virtual ~Event() {}
    void notifyAll(); // Notify all coroutines (add them to the runnable list)
    void wait(); // Add a coroutine to the wait set
    
    template <typename F> 
    void wait(F cond) {
        while (!cond()) {
            wait();
        }
    }

private:
    size_t waiters() const { return waiter_.size(); }
    EventWaitToken waitToken(Ptr<Coroutine> waiter);
    bool waitTokenValid(Ptr<Coroutine> waiter, EventWaitToken token);
    void waitTokenDel(EventWaitToken token);

    std::vector<EventRecord> waiter_;
    friend class Selector;
};

}
