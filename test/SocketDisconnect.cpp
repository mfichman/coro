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

#include <coro/Common.hpp>
#include <coro/coro.hpp>

auto msg = std::string("hello world\n");

coro::Ptr<coro::Socket> newServer() {
    auto ls = std::make_shared<coro::Socket>();
    ls->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
    ls->bind(coro::SocketAddr("127.0.0.1", 9090));
    ls->listen(10);
    
    auto sd = ls->accept();
    return sd;
}

coro::Ptr<coro::Socket> newClient() {
    auto sd = std::make_shared<coro::Socket>();
    sd->connect(coro::SocketAddr("127.0.0.1", 9090));
    return sd;
}

void testReadDisconnect() {
// Client disconnects while the server is reading from its socket.
    auto server = coro::start([]{
        char buf[1024];
        auto sd = newServer();
        sd->readAll(buf, msg.length()); 
        try {
            sd->readAll(buf, msg.length());     
            assert(!"failed");
        } catch (coro::SocketCloseException const& ex) {
        }
    });

    auto client = coro::start([]{
        auto sd = newClient();
        sd->writeAll(msg.c_str(), msg.length());
    });
    coro::run();
}

void testWriteDisconnect() {
// Server disconnects while the client is reading from its socket.
    auto server = coro::start([]{
        char buf[1024];
        auto sd = newServer();
        sd->readAll(buf, msg.length()); 
    });

    auto client = coro::start([]{
        auto sd = newClient();
        sd->writeAll(msg.c_str(), msg.length());
        try {
            while (true) {
                sd->writeAll(msg.c_str(), msg.length());
            }
            assert(!"failed");
        } catch (coro::SystemError const& ex) {
        }
    });
    coro::run();
}

int main() {
    testReadDisconnect();
    testWriteDisconnect();
    return 0;
}
