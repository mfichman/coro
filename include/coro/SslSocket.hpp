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

#pragma once

#include "coro/Common.hpp"
#include "coro/Socket.hpp"

namespace coro {

class SslError {
public:
    SslError(unsigned long error);
    std::string const& what() const { return what_; }

private:
    std::string what_;
};

class SslSocket : public Socket {
public:
    SslSocket(int type=SOCK_STREAM, int protocol=IPPROTO_TCP);
    virtual ~SslSocket();
    virtual ssize_t write(char const* buf, size_t len, int flags=0); // Execute 1 write() syscall
    virtual ssize_t read(char* buf, size_t len, int flags=0); // Execte 1 read() syscall

private:
    void writeAllRaw(char const* buf, size_t len, int flags);
    void writeFromBio(int flags);
    void readToBio(int flags);

    SSL_CTX* context_;
    SSL* conn_;
    BIO* in_;
    BIO* out_;
    bool eof_;
};

}
