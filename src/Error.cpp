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
#include "coro/Error.hpp"

namespace coro {

#ifdef _WIN32
std::string winErrorMsg(DWORD error) {
    std::string buffer(1024,'0');
    LPSTR buf = (LPSTR)buffer.c_str();
    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM;
    DWORD len = FormatMessage(flags, 0, error, 0, buf, buffer.size(), 0);
    buffer.resize(len);
    return buffer;
}
#endif

SystemError::SystemError() : 
#ifdef _WIN32
    SystemError(GetLastError()) {
#else
    SystemError(errno) {
#endif
    
}

SystemError::SystemError(int error) :
#ifdef _WIN32   
    error_(error),
    msg_(winErrorMsg(error_)) {
#else
    error_(error),
    msg_(strerror(error_)) {
#endif
}

}
