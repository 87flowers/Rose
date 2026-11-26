#pragma once

#include "lps2/vector.hpp"

#include <array>
#include <cstring>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> load(const void* src) {
    vector<T, N> result;
    std::memcpy(&result, src, sizeof(result));
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::load(const void* src) {
    return generic::vector_ops::load<T, N>(src);
  }
}  // namespace lps2
