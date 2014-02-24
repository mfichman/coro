/*
 * Copyright (c) 2014 Matt Fichman
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
#include "coro/Selector.hpp"
#include "coro/Coroutine.hpp"
#include "coro/Event.hpp"

namespace coro {

SelectorRecord::SelectorRecord(Ptr<Event> event, SelectorFunc func, int token) {
    event_ = event;
    func_ = func;
    token_ = token;
}

Selector::~Selector() {
    current()->wait();
    for (auto record : record_) {
        if (record.event()->waitTokenValid(current(), record.token())) {
            record.event()->waitTokenDel(record.token()); 
        } else {
            record.func()();
        }
    }
}

Selector& Selector::on(Ptr<Event> event, SelectorFunc func) {
    record_.push_back(SelectorRecord(event, func, event->waitToken(current())));
    return *this;
}


}
