#include <thread>

#include "Logger.h"

int main() {
    log::info("main started!");
    int a = 100;
    bool b = false;
    double p = 123.4567;
    std::this_thread::sleep_for(std::chrono::milliseconds(122));
    log::info("a =", a);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    log::warn("b is", b);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    log::error("p is", p, "and b is", b);
}
