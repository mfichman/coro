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
#include "coro/Hub.hpp"
#include "coro/Error.hpp"

#ifdef __APPLE__
#include "Hub.osx.inl"
#elif defined(_WIN32)
#include "Hub.win.inl"
#else
#include "Hub.linux.inl"
#endif

namespace coro {

Ptr<Hub> hub() {
    static Ptr<Hub> hub(new Hub);
    return hub;
}

void run() {
    hub()->run();
}

Hub::Hub() : blocked_(0), handle_(0) {
// Creates a new hub to manage I/O and runnable coroutines.  The hub is
// responsible for scheduling coroutines to run.
#if defined(_WIN32)
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    // Disable error dialog boxes
    SetErrorMode(GetErrorMode()|SEM_NOGPFAULTERRORBOX); 
    if (WSAStartup(version, &data) != 0) {
        abort();
    }
    handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
#elif defined(__APPLE__)
    handle_ = kqueue();
#elif defined(__linux__)
    handle_ = epoll_create(1);
#endif
    if (!handle_) {
        throw SystemError();
    }
}

void Hub::timeoutIs(Timeout const& timeout) {
// Schedules a coroutine to run in the future
    timeout_.push(timeout);
}

void Hub::quiesce() {
// Run coroutines until they are all blocked on I/O or dead.
    std::vector<WeakPtr<Coroutine>> runnable;
    runnable.swap(runnable_);
    for (auto weak : runnable) {
        auto coroutine = weak.lock();
        if (!coroutine) { continue; }
        main()->status_ = Coroutine::SUSPENDED;
        assert(coroutine->status()!=Coroutine::DEAD);
        coroutine->swap();
        switch (coroutine->status()) {
        case Coroutine::DEAD: break;
        case Coroutine::DELETED: break;
        case Coroutine::SUSPENDED:
            runnable_.push_back(coroutine);
            break;  
        case Coroutine::BLOCKED:
            blocked_++;
            break;
        case Coroutine::NEW: // fallthrough
        case Coroutine::RUNNING: // fallthrough
        default: assert(!"illegal coroutine state"); break;
        }
    }
}

void Hub::run() {
// Run coroutines and handle I/O until the process exits
    assert(coro::current() == coro::main());
    while (true) {
        auto const now = Time::now();
        while (!timeout_.empty() && timeout_.top().time() <= now) {
            auto const timeout = timeout_.top();
            auto const coro = timeout.coroutine();
            coro->status_ = Coroutine::SUSPENDED;
            runnable_.push_back(coro);
            timeout_.pop();
        }
        quiesce();
        poll();
    }
}

}
