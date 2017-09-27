#ifndef __EXPECTED_HPP__
#define __EXPECTED_HPP__

#include <cassert>
#include <utility>

enum class ErrorCode {
    NO_ERROR = 0,

    EC_INVALID_INPUT = 0x10,
    // add new error code here
};

template <typename T, typename E = ErrorCode> struct Expected {
private:
  T value;
  E error = {};
  mutable bool checked = false;

public:
  Expected(Expected const&) = default;
  Expected(Expected&& o) { this->operator=(std::move(o)); }

  Expected& operator=(Expected const&) = default;
  Expected& operator=(Expected&& o) {
    this->value = std::move(o.value);
    this->error = std::move(o.error);
    this->checked = std::move(o.checked);
    o.checked = true;
    return *this;
  }

  Expected(T const& value) : value{value} {}
  Expected(T&& value) : value{std::move(value)} {}
  Expected(E error) : error{error} {}

  ~Expected() { assert(this->checked); }

  T& get() {
    assert(this->checked);
    return value;
  }

  E getError() const {
    this->checked = true;
    return error;
  }
};

template <typename E> struct Expected<void, E> : Expected<int, E> {
  using Base = Expected<int, E>;
  using Base::Base;

  Expected(int) = delete;
};

template <typename E, typename... Args>
static E MakeUnexpected(E&& error, Args&&... args) {
  if (sizeof...(args) > 0)
    print_error(std::forward<Args>(args)...);
  errno = 0;
  return std::forward<E>(error);
}

#endif //__EXPECTED_HPP__
