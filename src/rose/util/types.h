#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#define forceinline inline __attribute__((always_inline))

namespace rose::vec {
  union v128;
  union v256;
  union v512;
}; // namespace rose::vec

namespace rose {
  using u8 = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;
  using u128 = unsigned __int128;

  using i8 = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;
  using i128 = __int128;

  using isize = std::intptr_t;
  using usize = std::size_t;
  static_assert(sizeof(isize) == sizeof(usize));

  using f32 = float;
  using f64 = double;

  using v128 = vec::v128;
  using v256 = vec::v256;
  using v512 = vec::v512;

  namespace time {
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = TimePoint::duration;
    using FloatSeconds = std::chrono::duration<f64>;
    using Milliseconds = std::chrono::duration<i64, std::milli>;

    template <typename T> constexpr auto cast(const auto &d) -> T { return std::chrono::duration_cast<T>(d); }
    template <typename T> constexpr auto nps(u64 nodes, const auto &elapsed) -> T {
      static_assert(std::is_same_v<T, u64> || std::is_same_v<T, f64>);
      return static_cast<T>(static_cast<f64>(nodes) / cast<FloatSeconds>(elapsed).count());
    }
  } // namespace time

  using Score = i32;

} // namespace rose
