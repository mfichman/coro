#include <coro/Socket.hpp>
#include <coro/Hub.hpp>
#include <coro/Error.hpp>
#include <iostream>

void server() {
    try {
        char buf[1024];
        auto ls = std::make_shared<coro::Socket>();
        ls->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        ls->bind(coro::SocketAddr("127.0.0.1", 9090));
        ls->listen(10);

        auto sd = ls->accept();
        while (auto len = sd->read(buf, sizeof(buf))) {
            printf("%*s", (int)len, buf);
        }
        exit(0);
    } catch (coro::SystemError const& ex) {
        std::cout << ex.what() << std::endl;
        exit(1);
    }
}

void client() {
    auto sd = std::make_shared<coro::Socket>();
    auto msg = "hello world\n";
    try {
        sd->connect(coro::SocketAddr("127.0.0.1", 9090));
        for (auto i = 0; i < 1000; ++i) {
            sd->write(msg, strlen(msg));
        } 
    } catch (coro::SystemError const& ex) {
        std::cout << ex.what() << std::endl;
    }
}

int main() {
    coro::start(server);
    coro::start(client);
    coro::run();

    return 0;
}
