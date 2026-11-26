#pragma once

#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> nonzeros_vm(const vector<T, N>& first) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, first_raw[i]);
    }
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr usize nonzeros_count(const vector<T, N>& first) {
    usize result = 0;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result += first_raw[i] != 0;
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::nonzeros_vm() const {
    return neq(zero());
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::nonzeros_bm() const {
    return nonzeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::nonzeros() const {
    return nonzeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr usize vector<T, N>::nonzeros_count() const {
    return nonzeros_vm().popcount();
  }
}  // namespace lps2
