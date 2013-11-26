
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

class Time {
public:
    static Time sec(int64_t sec) { return Time(sec*1000000); }
    static Time millisec(int64_t ms) { return Time(ms*1000); }
    static Time microsec(int64_t us) { return Time(us); }
    static Time now();

    bool operator<(Time const& rhs) const { return microsec_ < rhs.microsec_; }
    bool operator>(Time const& rhs) const { return microsec_ > rhs.microsec_; }
    bool operator==(Time const& rhs) const { return microsec_ == rhs.microsec_; }
    bool operator<=(Time const& rhs) const { return !operator>(rhs); }
    bool operator>=(Time const& rhs) const { return !operator<(rhs); }
    bool operator!=(Time const& rhs) const { return !operator==(rhs); }
    Time operator-(Time const& rhs) const { return Time(microsec_ - rhs.microsec_); }
    Time operator+(Time const& rhs) const { return Time(microsec_ + rhs.microsec_); }

#ifdef _WIN32
    struct timespec timespec() const;
#endif
    int64_t millisec() const { return microsec_/1000; }
    int64_t microsec() const { return microsec_; }
private:
    Time(int64_t microsec) : microsec_(microsec) {}
    int64_t microsec_;
};

}
