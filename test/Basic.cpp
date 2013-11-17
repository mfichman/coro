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
#include "coro/Coroutine.hpp"
#include "coro/Hub.hpp"

char* recurse(int foo) {
    if(foo ==0) { return 0; }
    char buf[1024];
    char* buf2 = buf;
    recurse(foo-1);
    return buf2;
}

void foo() {
//    recurse(900);

    try {
        std::cout << "hello" << std::endl;
        coro::yield();
    } catch (coro::ExitException const& ex) {
        std::cout << "exception" << std::endl;
        throw;
    }
    std::cout << "hello" << std::endl;

    coro::sleep(coro::Time::sec(4));
    std::cout << "one\n" << std::endl;
    coro::sleep(coro::Time::sec(4));
    std::cout << "two\n" << std::endl;
}

void bar() {
    while (true) {
    coro::sleep(coro::Time::millisec(1000));
    std::cout << "barrrrrr" << std::endl;
    }
}

void baz() {
    while (true) {
    coro::sleep(coro::Time::millisec(100));
    std::cout << "baz" << std::endl;
    }
}

int main() {
    coro::start(baz);
    coro::start(bar);
    coro::start(foo);
    coro::run();
    return 0;
}
