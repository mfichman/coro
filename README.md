coro
====

Modern coroutine library for C++, including an async socket I/O implementation (with SSL support).

Basic usage:

```
auto c = coro::start([]() {
   std::cout << "I'm a coroutine!" << std::endl;
   coro::sleep(coro::Time::millisec(100));
});
coro::run(); // runs the coroutine dispatch loop
```

Support for OS X and Windows!
