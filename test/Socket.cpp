#include <coro/Common.hpp>
#include <coro/coro.hpp>
#include <iostream>

void server() {
    try {
        char buf[1024];
        auto ls = std::make_shared<coro::Socket>();
        ls->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        ls->bind(coro::SocketAddr("127.0.0.1", 9090));
        ls->listen(10);

        auto sd = ls->accept();
        ssize_t len = 0;
        while ((len = sd->read(buf, sizeof(buf))) > 0) {
            printf("%.*s", (int)len, buf);
            fflush(stdout);
        }
        std::cout << "exit" << std::endl;
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
            sd->writeAll(msg, strlen(msg));
        } 
    } catch (coro::SystemError const& ex) {
        std::cout << ex.what() << std::endl;
    }
}

int main() {
    auto cserver = coro::start(server);
    auto cclient = coro::start(client);
    coro::run();
    return 0;
}
