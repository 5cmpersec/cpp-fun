#ifndef __SCOPED_TIMER_HPP__
#define __SCOPED_TIMER_HPP__

#include <chrono>
#include <iostream>

class ScopedTimer {
    private:
        const char* name;
        std::chrono::high_resolution_clock::time_point start;
    public:
        ScopedTimer(const char* name)
            : name(name), start(std::chrono::high_resolution_clock::now()) {}

        ~ScopedTimer() {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start);
            std::cout << name << " took " << duration.count() << " us" << std::endl;
        }
};

#endif //__SCOPED_TIMER_HPP__
