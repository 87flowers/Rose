#pragma once

#include "rose/common.hpp"

#include <chrono>
#include <type_traits>

namespace rose::time {
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  using Duration = TimePoint::duration;
  using FloatSeconds = std::chrono::duration<f64>;
  using Milliseconds = std::chrono::duration<i64, std::milli>;

  template<typename T>
  constexpr auto cast(const auto& d) -> T {
    return std::chrono::duration_cast<T>(d);
  }

  template<typename T>
  constexpr auto nps(u64 nodes, const auto& elapsed) -> T {
    static_assert(std::is_same_v<T, u64> || std::is_same_v<T, f64>);
    return static_cast<T>(static_cast<f64>(nodes) / cast<FloatSeconds>(elapsed).count());
  }
}  // namespace rose::time
