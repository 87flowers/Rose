#pragma once

#include "lps2/bit_mask.hpp"

#include <bit>

namespace lps2 {

  template<class T, usize N>
  [[nodiscard]] LPS2_INLINE constexpr usize basic_bit_mask<T, N>::popcount() const {
    return static_cast<usize>(std::popcount(to_bits()));
  }

}  // namespace lps2
