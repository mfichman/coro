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

    current()->block();    

    // Check for connect error code
    if (::read(sd_, 0, 0) < 0) {
        throw SystemError();
    } 
}

Ptr<Socket> Socket::accept() {
// Accept a new incoming connection asynchronously. Register to wait for a READ
// event, which signals that we can call accept() without blocking.
    int kqfd = hub()->handle();
    int flags = EV_ADD|EV_ONESHOT;
    struct kevent ev{0};
    EV_SET(&ev, sd_, EVFILT_READ, flags, 0, 0, current().get());
    if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
        throw SystemError();
    }
    // Wait until the socket becomes readable.  At that point, there will be a
    // peer waiting in the accept queue.
    current()->block();

    // Accept the peer, and create a new stream socket.
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int sd = ::accept(sd_, (struct sockaddr*)&sin, &len);
    if (sd < 0) {
        throw SystemError();
    }  
    if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
        throw SystemError();
    }
    return Ptr<Socket>(new Socket(sd, ""));
}

ssize_t Socket::read(char* buf, size_t len, int flags) {
// Read from the socket asynchronously
    while (true) {
        auto ret = recv(sd_, buf, len, flags);
        if (ret < 0 && errno != EAGAIN) {
            throw SystemError();
        } else if (ret >= 0) {
            return ret;
        }
        int kqfd = hub()->handle();
        int kqflags = EV_ADD|EV_ONESHOT|EV_EOF;
        struct kevent ev{0};
        EV_SET(&ev, sd_, EVFILT_READ, kqflags, 0, 0, current().get()); 
        if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
            throw SystemError();
        }
        current()->block();
    }
    return 0;
}

ssize_t Socket::write(char const* buf, size_t len, int flags) {
// Write asynchronously
    size_t total = 0;
    while (true) {
        auto ret = send(sd_, buf, len, flags);
        if (ret < 0 && errno != EAGAIN) {
            throw SystemError();
        } else if (ret == 0) {
            return total;
        } else {
            len -= ret;
            buf += ret;
            total += ret; 
        }
        if (len == 0) {
            return;
        }
        int kqfd = hub()->handle();
        int kqflags = EV_ADD|EV_ONESHOT|EV_EOF;
        struct kevent ev{0};
        EV_SET(&ev, sd_, EVFILT_WRITE, kqflags, 0, 0, current().get()); 
        if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
            throw SystemError();
        }
        current()->block();
    }
    return total;
}


}
