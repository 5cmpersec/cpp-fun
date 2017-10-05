#ifndef __LOGGER_LOGGER_H__
#define __LOGGER_LOGGER_H__

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cerrno>
#include <cstring>

namespace log {
    extern std::chrono::steady_clock::time_point g_ref_point;

    inline std::ostream& operator<<(std::ostream& out, std::chrono::steady_clock::time_point const& tp) {
        auto elapsed = std::chrono::duration<double>(tp - g_ref_point);
        return out << "[" << std::fixed << std::setprecision(4) << elapsed.count() << "]";
    }

    template <typename... Ts>
        std::ostream& join(std::ostream& str, char const* sep, Ts&&... as) {
            auto n = sizeof...(as);
            (void)std::initializer_list<int>{
                (str << std::forward<Ts>(as) << (--n == 0 ? "" : sep), 0)...};
            return str;
        }

    template <typename... Ts> std::string join(char const* sep, Ts&&... as) {
        return join(std::stringstream{}, sep, std::forward<Ts>(as)...).str();
    }

    template <typename... Ts> void error(Ts&&... as) {
        auto now = std::chrono::steady_clock::now();
        join(std::clog, " ", now, "E:", std::forward<Ts>(as)...)
            << " [errno: " << errno << " - " << std::strerror(errno) << "]" << std::endl;
    }

    template <typename... Ts> void warn(Ts&&... as) {
        auto now = std::chrono::steady_clock::now();
        join(std::clog, " ", now, "W:", std::forward<Ts>(as)...) << std::endl;
    }

    template <typename... Ts> void info(Ts&&... as) {
        auto now = std::chrono::steady_clock::now();
        join(std::clog, " ", now, "I:", std::forward<Ts>(as)...) << std::endl;
    }

}

#endif //__LOGGER_LOGGER_H__
