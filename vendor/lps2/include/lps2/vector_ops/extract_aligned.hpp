#pragma once

#include "lps2/vector.hpp"

#include <cstdlib>

namespace lps2::generic::vector_ops {
  template<class V, usize extract_index, class T, usize N>
  LPS2_INLINE constexpr V extract_aligned(const vector<T, N>& self) {
    V result;
    std::memcpy(&result, reinterpret_cast<const char*>(&self) + extract_index * sizeof(V), sizeof(V));
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  template<class V, usize extract_index>
  LPS2_INLINE constexpr V vector<T, N>::extract_aligned() const {
    return generic::vector_ops::extract_aligned<V, extract_index>(*this);
  }
}  // namespace lps2
