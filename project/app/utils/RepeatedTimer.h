#pragma once

#include <iostream>
#include <asio.hpp>

class Loop
{
private:
    using duration = std::chrono::steady_clock::duration;

    duration period_;
    asio::steady_timer timer_;

    void callback(std::error_code)
    {
        std::cout << "Hello world!\n";
        timer_.expires_after(period_);
        timer_.async_wait(
            std::bind_front(&Loop::callback, this));
    }

public:
    explicit Loop(asio::io_context& ioc,
        const duration dur):
        period_(dur), timer_(ioc, dur) {}

    void start()
    {
        timer_.async_wait(
            std::bind_front(&Loop::callback, this));
    }
};

//int main()
//{
//    using namespace std::literals;
//
//    asio::io_context context;
//
//    Loop loop(context, 5s);
//    loop.start();
//    // do work
//
//
//    context.run();
//
//    return 0;
//}
