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
// Record to track when a coroutine should be scheduled in the future.  When
// the time elapses, the coroutine is resumed.
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

#ifdef _WIN32
struct Overlapped {
    OVERLAPPED overlapped;
    Coroutine* coroutine;
    DWORD bytes;
    DWORD error;
};
#endif

class Hub {
// The hub manages all coroutines, events, and I/O.  It runs an event loop that
// schedules coroutines to run when the events they are waiting on (channel,
// I/O, etc.) have signaled.
public:
    template <typename F>
    Ptr<Coroutine> start(F func) { 
        Ptr<Coroutine> coro(new Coroutine(func));
        runnable_.push_back(coro);
        return coro;
    }
    void quiesce();
    void poll();
    void run();
#ifdef _WIN32
    HANDLE handle() const { return handle_; }
#else
    int handle() const { return handle_; }
#endif
    std::mutex const& mutex() const { return mutex_; }

private:
    Hub();
    std::vector<WeakPtr<Coroutine>> runnable_;
    std::priority_queue<Timeout, std::vector<Timeout>> timeout_;
    int blocked_;
    int waiting_;
#ifdef _WIN32
    HANDLE handle_;
#else
    int handle_; 
#endif
    std::mutex mutex_;

    void timeoutIs(Timeout const& timeout);

    friend Ptr<Hub> coro::hub();
    friend void coro::sleep(Time const& time);
    friend class Coroutine;
    friend class Event;
};

template <typename F>
Ptr<Coroutine> start(F func) {
    return hub()->start(func);
}

template <typename F>
Ptr<Coroutine> timer(F func, Time const& time) {
    return start([=] { 
        std::function<void()> f = func;
        while (true) {
            f();
            coro::sleep(time); 
        }
    });
}

}
