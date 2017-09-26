#ifndef __SCOPE_GUARD_HPP__
#define __SCOPE_GUARD_HPP__

#include <utility>

template <typename L> struct ScopeGuard {
      ~ScopeGuard() { this->lambda(); }
        L lambda;
};

template <typename F> static ScopeGuard<F> MakeScopeGuard(F&& f) {
      return ScopeGuard<F>{std::forward<F>(f)};
};

#endif //__SCOPE_GUARD_HPP__
