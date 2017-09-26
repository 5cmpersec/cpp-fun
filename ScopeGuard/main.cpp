#include <cassert>
#include "ScopeGuard.hpp"

int main(int argc, char const* argv[]) {
    auto buff = new char[1024];
    auto buff_guard = MakeScopeGuard([buff] { delete[] buff; });
    assert(buff != nullptr);
}
