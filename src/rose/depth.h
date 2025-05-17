#pragma once

#include "rose/util/types.h"

namespace rose {

  struct Depth {
    static inline constexpr i32 granularity = 64;

    i32 raw = 0;

    static constexpr auto zero() -> Depth { return {0}; }
    static constexpr auto one() -> Depth { return {granularity}; }
    static constexpr auto fromInt(i32 value) -> Depth { return {value * granularity}; }

    // Round-to-zero
    constexpr auto rtz() const -> i32 { return raw / granularity; }

    constexpr auto operator+(Depth other) const -> Depth { return {raw + other.raw}; }
    constexpr auto operator-(Depth other) const -> Depth { return {raw - other.raw}; }
    constexpr auto operator+=(Depth other) -> Depth & {
      raw += other.raw;
      return *this;
    }
    constexpr auto operator-=(Depth other) -> Depth & {
      raw -= other.raw;
      return *this;
    }

    constexpr auto operator<=>(const Depth &) const -> std::strong_ordering = default;
  };
} // namespace rose
