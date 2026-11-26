#pragma once

#include "lps2/detail/mask_element.hpp"

namespace lps2 {

  template<class T, usize N>
  struct basic_bit_mask;

  template<class T, usize N>
  struct basic_vector_mask;

  template<class T, usize N>
  struct vector;

  template<class T, usize N>
  using bit_mask = basic_bit_mask<detail::mask_element_t<T>, N>;

  template<class T, usize N>
  using vector_mask = basic_vector_mask<detail::mask_element_t<T>, N>;

};  // namespace lps2
