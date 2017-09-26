#include <chrono>
#include <thread>

#include "ScopedTimer.hpp"

int main() {
    auto t = ScopedTimer("[main function]");
    std::this_thread::sleep_for(std::chrono::milliseconds(969));

    return 1;
}
