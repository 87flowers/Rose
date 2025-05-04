#pragma once

namespace rose::internal {
  template <typename F> struct DeferHelper {
    F f;
    DeferHelper(F &&f) : f(std::forward<F>(f)) {}
    ~DeferHelper() { f(); }
  };
} // namespace rose::internal

#define rose_defer const rose::internal::DeferHelper _ = [&]() -> void
