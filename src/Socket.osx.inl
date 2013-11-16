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

void Socket::connect(SocketAddr const& addr) {
// Connect this socket to a remote socket
    if (fcntl(sd_, F_SETFL, O_NONBLOCK) < 0) {
        throw SystemError();
    }

    // Set the socket in non-blocking mode, so that the call to connect() below
    // and calls to send/recv do not block.
    struct sockaddr_in sin = addr.sockaddr();
    int ret = ::connect(sd_, (struct sockaddr*)&sin, sizeof(sin));
    if (ret < 0 && errno != EINPROGRESS) {
        throw SystemError();
    }

    int kqfd = hub()->handle();
    int flags = EV_ADD|EV_ONESHOT|EV_EOF;
    struct kevent ev{0};
    EV_SET(&ev, sd_, EVFILT_WRITE, flags, 0, 0, current().get());
    if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
        throw SystemError();
    }

    Ptr<Coroutine> anchor = current();
    yield();    

    // Check for connect error code
    if (::read(sd_, 0, 0) < 0) {
        throw SystemError();
    } 
}


}
