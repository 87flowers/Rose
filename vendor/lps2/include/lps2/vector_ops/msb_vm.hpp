#pragma once

#include "lps2/detail/msb.hpp"
#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>
#include <climits>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> msb_vm(const vector<T, N>& first) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, detail::msb(first_raw[i]));
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::msb_vm() const {
    constexpr T msb_bit = static_cast<T>(1) << (sizeof(T) * CHAR_BIT - 1);
    return test_vm(vector::splat(msb_bit));
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::msb_bm() const {
    return msb_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::msb() const {
    return msb_vm();
  }
}  // namespace lps2
