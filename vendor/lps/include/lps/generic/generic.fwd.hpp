#pragma once

#include "lps/detail/mask_element.hpp"
#include "lps/stdint.hpp"

namespace lps::generic {

  template<class T, usize N>
  struct vector;

  template<class T, usize N>
  struct basic_vector_mask;

  template<class T, usize N>
  using vector_mask = basic_vector_mask<detail::mask_element_t<T>, N>;

}  // namespace lps::generic
