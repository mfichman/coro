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

void Hub::poll() {
// Pool for I/O events.  If there are pending coroutines, then don't block
// indefinitely -- just check for any ready I/O.  If there are timers, block
// only until the min timer is ready.
    size_t tasks = runnable_.size()+timeout_.size();
    struct timespec timeout{0};
    struct kevent event{0};

    if (!timeout_.empty() && runnable_.empty()) {
        auto const diff = timeout_.top().time()-Time::now();
        if (diff > Time::sec(0)) {
            timeout = diff.timespec();
        }
    }

    int res = kevent(handle_, 0, 0, &event, 1, (tasks <= 0 ? 0 : &timeout));
    //self->iobytes = event.data;
    if (res < 0) {
        throw SystemError();
    } else if (res == 0) {
        // No events
    } else {
        auto const coro = (Coroutine*)event.udata;
        assert(coro->status()!=Coroutine::DEAD);
        coro->status_ = Coroutine::SUSPENDED;
        runnable_.push_back(coro->shared_from_this());
        blocked_--;
    }
}

}
