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
#include "coro/Socket.hpp"
#include "coro/Coroutine.hpp"
#include "coro/Hub.hpp"
#include "coro/Error.hpp"

#ifdef __APPLE__
#include "Socket.osx.inl"
#elif defined(_WIN32)
#include "Socket.win.inl"
#else
#include "Socket.linux.inl"
#endif

namespace coro {

struct in_addr SocketAddr::inaddr() const {
// Attempts to translate from the input string as if it were a dotted quad
// first.  If this fails, then assume that the address string is a DNS name,
// and do a DNS lookup.
    struct in_addr in{0};
    if (host().empty()) {
        return in;
    }
    if (inet_pton(AF_INET, (char*)host().c_str(), &in) == 1) { 
        return in;
    }
    struct addrinfo* res = 0;
    auto ret = getaddrinfo(host().c_str(), 0, 0, &res);
    if (ret) {
        throw SystemError(gai_strerror(ret));
    }
    for(struct addrinfo* addr = res; addr; addr = addr->ai_next) {
        struct sockaddr_in* sin = (struct sockaddr_in*)addr->ai_addr;
        if (sin->sin_addr.s_addr) {
            in = sin->sin_addr;
            freeaddrinfo(res);
            return in;
        }
    }
    freeaddrinfo(res);
    assert(!"no addresses found");
    return in;
}

struct sockaddr_in SocketAddr::sockaddr() const {
// Returns the socket addr used by the low-level socket API 
    struct sockaddr_in sin{0};
    sin.sin_family = AF_INET;
    sin.sin_addr = inaddr();
    sin.sin_port = htons(port());
    return sin;
}

Socket::Socket(int type, int protocol) : sd_(0) {
// Creates a new socket; throws a socket exception if creation fails
    hub(); // Make sure the hub is active
    sd_ = socket(AF_INET, type, protocol);
    if(sd_<0) {
        throw SystemError();
    }
#ifdef _WIN32
    if(!CreateIoCompletionPort((HANDLE)sd_, hub()->handle(), 0, 0)) {
        throw SystemError();
    }
#else
    setsockopt(SOL_SOCKET, SO_NOSIGPIPE, true);    
    // Don't send SIGPIPE for this socket; handle the write() error instead.
#endif
}

Socket::Socket(int sd, char const* /* bogus */) : sd_(sd) {
#ifdef _WIN32
    if(!CreateIoCompletionPort((HANDLE)sd_, hub()->handle(), 0, 0)) {
        throw SystemError();
    }
#else
    setsockopt(SOL_SOCKET, SO_NOSIGPIPE, true); 
    // Don't send SIGPIPE for this socket; handle the write() error instead.
#endif
}

void Socket::bind(SocketAddr const& addr) {
// Binds the socket to a port
    struct sockaddr_in sin = addr.sockaddr();
    if (::bind(sd_, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        throw SystemError();
    }
}

void Socket::listen(int backlog) {
    if (::listen(sd_, backlog)) {
        throw SystemError();
    }
}

void Socket::setsockopt(int level, int option, int value) {
    if (::setsockopt(sd_, level, option, (char*)&value, sizeof(value))) {
        throw SystemError();
    }
}

void Socket::writeAll(char const* buf, size_t len, int flags) {
// Write all data in 'buf'.  Block until all data is written, or the connection
// is closed.  Throws a SocketCloseException if the connection was closed by
// the other end.
    while(len > 0) {
        ssize_t bytes = write(buf, len, flags);
        if (bytes == 0) {
            throw SocketCloseException(); 
        } else if (bytes > 0) {
            buf += bytes;
            len -= bytes;
        } else {
            assert(!"unexpected negative byte value");
        }
    }
}

void Socket::readAll(char* buf, size_t len, int flags) {
// Read all data in 'buf'.  Block until all data is read, or the connection is
// closed.  Throws a SocketCloseException if the connection was closed by the
// other end.
    while(len > 0) {
        ssize_t bytes = read(buf, len, flags);
        if (bytes == 0) {
            throw SocketCloseException();
        } else if (bytes > 0) {
            buf += bytes;
            len -= bytes;
        } else {
            assert(!"unexpected negative byte value");
        }
    }
}


void Socket::close() {
#ifdef _WIN32
    ::closesocket(sd_);
#else
    ::close(sd_);
#endif
    sd_ = -1;
}

}
