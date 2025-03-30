#pragma once

namespace rose::internal {
  template <typename F> struct DeferHelper {
    F f;
    DeferHelper(F &&f) : f(std::forward<F>(f)) {}
    ~DeferHelper() { f(); }
  };
} // namespace rose::internal

#define ROSE_CONCAT2(x, y) x##y
#define ROSE_CONCAT(x, y) ROSE_CONCAT2(x, y)

#define rose_defer const rose::internal::DeferHelper ROSE_CONCAT(_rose_internal_defer_guard_, __COUNTER__) = [&]() -> void
