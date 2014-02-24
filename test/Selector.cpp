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


#include <coro/coro.hpp>

using namespace coro;

Ptr<Event> e1(new Event);
Ptr<Event> e2(new Event);
Ptr<Event> e3(new Event);

void publisher() {
    e1->notifyAll();
    e2->notifyAll();

}

void consumer() {
    int count = 2;
    while (count > 0) {
        coro::Selector()
            .on(e1, [&]() { count--; std::cout << "e1" << std::endl; })
            .on(e2, [&]() { count--; std::cout << "e2" << std::endl; })
            .on(e3, [&]() { std::cout << "e2" << std::endl; });
    }
}

int main() {
    auto b = coro::start(consumer);
    auto a = coro::start(publisher);

    coro::run();

    return 0;
}
