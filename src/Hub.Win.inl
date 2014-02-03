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
// Poll For I/O events.  If there are pending coroutines, then don't block
// indefinitely -- just check for any ready I/O.  If there are timers, block
// only until the min timer is ready.
    size_t tasks = runnable_.size()+timeout_.size();
    DWORD timeout = 0;
    if (!timeout_.empty() && runnable_.empty()) {
        auto const diff = timeout_.top().time()-Time::now();
        if (diff > Time::sec(0)) {
            timeout = diff.millisec();
        }
    }
    if (tasks <= 0) {
        timeout = INFINITE;
    }

    SetLastError(ERROR_SUCCESS);
    ULONG_PTR udata = 0;
    Overlapped* op = 0;
    OVERLAPPED** evt = (OVERLAPPED**)&op;
    DWORD bytes = 0;
    std::cout << "timeout=" << timeout << std::endl; 
    BOOL ret = GetQueuedCompletionStatus(handle_, (LPDWORD)&bytes, &udata, evt, timeout);
    if (!op) { return; }

    if (ret) {
        op->bytes = bytes;
        op->error = ERROR_SUCCESS;
    } else {
        op->bytes = 0;
        op->error = GetLastError();
    }
    auto const coro = (Coroutine*)op->coroutine;
    assert(coro->status()!=Coroutine::DEAD);
    coro->unblock();
}

}
