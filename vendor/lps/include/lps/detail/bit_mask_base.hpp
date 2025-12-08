#pragma once

#include "lps/stdint.hpp"

namespace lps::detail {

  template<usize N>
  struct bit_mask_base;

  template<>
  struct bit_mask_base<2> {
    using type = u8;
  };

  template<>
  struct bit_mask_base<4> {
    using type = u8;
  };

  template<>
  struct bit_mask_base<8> {
    using type = u8;
  };

  template<>
  struct bit_mask_base<16> {
    using type = u16;
  };

  template<>
  struct bit_mask_base<32> {
    using type = u32;
  };

  template<>
  struct bit_mask_base<64> {
    using type = u64;
  };

  template<usize N>
  using bit_mask_base_t = typename bit_mask_base<N>::type;

}  // namespace lps::detail
