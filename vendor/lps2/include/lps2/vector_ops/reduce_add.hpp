#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr T reduce_add(const vector<T, N>& self) {
    T result = 0;
    std::array<T, N> self_raw = self.to_array();
    for (usize i = 0; i < N; i++) {
      result += self_raw[i];
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr T vector<T, N>::reduce_add() const {
#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vaddvq_s8(raw);
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vaddvq_s16(raw);
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vaddvq_s32(raw);
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vaddvq_s64(raw);
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vaddvq_u8(raw);
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vaddvq_u16(raw);
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vaddvq_u32(raw);
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vaddvq_u64(raw);
      }
    }
#endif
    return generic::vector_ops::reduce_add(*this);
  }
}  // namespace lps2
