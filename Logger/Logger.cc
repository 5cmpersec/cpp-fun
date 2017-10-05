#include "Logger.h"

namespace log {
    std::chrono::steady_clock::time_point g_ref_point = std::chrono::steady_clock::now();
}
