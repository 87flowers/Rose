#pragma once

#include "lps2/vector.hpp"

#include <cstdlib>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr T read(const vector<T, N>& self, usize i) {
    T value;
    std::memcpy(&value, reinterpret_cast<const char*>(&self) + i * sizeof(T), sizeof(T));
    return value;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr T vector<T, N>::read(usize i) const {
    return generic::vector_ops::read(*this, i);
  }
}  // namespace lps2
