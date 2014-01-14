
#include <coro/Common.hpp>
#include <coro/coro.hpp>

int main() {
    auto event = coro::Event();
    auto trigger = false;

    auto notifier = coro::start([&]() {
        printf("notified\n");
        trigger = true;
        event.notifyAll();
    });

    auto waiter = coro::start([&]() {
        printf("waiting\n");
        event.wait([&]() { return trigger; }); 
        printf("done\n");
    });

    coro::run();

    return 0;
}
