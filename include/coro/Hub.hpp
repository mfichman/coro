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
#include "coro/Coroutine.hpp"

namespace coro {

Ptr<Hub> hub();
void run();

class Timeout {
public:
    Timeout(Time const& time, Ptr<Coroutine> coro) : time_(time), coroutine_(coro) {}
    bool operator<(Timeout const& rhs) const { return time_ > rhs.time_; }
    bool operator==(Timeout const& rhs) const { return time_ == rhs.time_; }

    Time const& time() const { return time_; }
    Ptr<Coroutine> coroutine() const { return coroutine_; }
private:
    Time time_;
    Ptr<Coroutine> coroutine_;
};

class Hub {
// The hub manages all coroutines, events, and I/O.  It runs an event loop that
// schedules coroutines to run when the events they are waiting on (channel,
// I/O, etc.) have signaled.
public:
    template <typename F>
    void start(F func) { 
        runnable_.push_back(Ptr<Coroutine>(new Coroutine(func)));
    }
    void quiesce();
    void poll();
    void run();
    int handle() const { return handle_; }
    std::mutex const& mutex() const { return mutex_; }

private:
    Hub();
    std::vector<Ptr<Coroutine>> runnable_;
    std::priority_queue<Timeout, std::vector<Timeout>> timeout_;
    int blocked_;
    int handle_; 
    std::mutex mutex_;

    void timeoutIs(Timeout const& timeout);

    friend Ptr<Hub> coro::hub();
    friend void coro::sleep(Time const& time);
    friend class Coroutine;
};

template <typename F>
void start(F func) {
    hub()->start(func);
}

}
