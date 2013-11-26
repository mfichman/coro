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
#include "coro/Time.hpp"

namespace coro {

Time Time::now() {
#ifdef _WIN32
    LARGE_INTEGER count{0};
    QueryPerformanceCounter(&count);
    LARGE_INTEGER freq{0};
    QueryPerformanceFrequency(&freq);
    return Time::microsec(1000000 * count.QuadPart / freq.QuadPart);
#else
    struct timeval ts{0};
    gettimeofday(&ts, 0);
    return Time::sec(ts.tv_sec)+Time::microsec(ts.tv_usec); 
#endif
}

#ifndef _WIN32
struct timespec Time::timespec() const {
// Convert to a timespec struct for use with various system calls
    struct timespec out{0};
    out.tv_sec = microsec_/1000000;
    out.tv_nsec = (microsec_%1000000)*1000;
    return out;
}
#endif

}
