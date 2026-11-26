#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr T reduce_xor(const vector<T, N>& self) {
    T result = 0;
    std::array<T, N> self_raw = self.to_array();
    for (usize i = 0; i < N; i++) {
      result ^= self_raw[i];
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr T vector<T, N>::reduce_xor() const {
    return generic::vector_ops::reduce_xor(*this);
  }
}  // namespace lps2
