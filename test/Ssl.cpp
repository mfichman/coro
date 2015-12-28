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
#include <sstream>

void testHttpGet() {
    auto sd = std::make_shared<coro::SslSocket>();
    sd->connect(coro::SocketAddr("www.google.com", 443));
    char const buf[] = "GET / HTTP/1.1\nHost: www.google.com\nAccept-Encoding:identity\nConnection: close\n\n";
    sd->writeAll(buf, sizeof(buf)-1); 

    char buf2[4096];
    std::stringstream ss;
    while (ssize_t len = sd->read(buf2, sizeof(buf2))) {
        std::cerr << std::string(buf2, len);
    }
}

void testClientServer() {

    auto c = coro::start([&]{
        auto ls = std::make_shared<coro::SslSocket>();
        ls->bind(coro::SocketAddr("127.0.0.1", 8000));
        ls->listen(10);
		ls->useCertificateFile("test/test.crt");
		ls->usePrivateKeyFile("test/test.key");

        auto sd = ls->accept();

		ls.reset();
        char const buf[] = "foobar\n";
        sd->writeAll(buf, sizeof(buf)-1);

		char buf2[4096];
		while (sd->read(buf2, sizeof(buf2))) {
		}
    });

    auto s = coro::start([&]{
        auto sd = std::make_shared<coro::SslSocket>();
        sd->connect(coro::SocketAddr("127.0.0.1", 8000));

        char const buf[] = "GET / HTTP/1.1\nHost: 127.0.0.1\nConnection: close\n\n";
        sd->writeAll(buf, sizeof(buf)-1); 
		sd->shutdown(SHUT_WR);
    
        char buf2[4096];
        std::stringstream ss;
        while (ssize_t len = sd->read(buf2, sizeof(buf2))) {
            std::cerr << std::string(buf2, len);
        }
    });

    s->join();
}

void test() {

    //testHttpGet();
    testClientServer();
}

int main() {
    
    auto c = coro::start(test);
    coro::run();

    return 0;
}
