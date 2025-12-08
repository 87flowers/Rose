#pragma once

#include "lps/detail/mask_element.hpp"
#include "lps/stdint.hpp"

namespace lps::doubling {

  template<class T, usize N, class Env>
  struct vector;

  template<class T, usize N, class Env>
  struct basic_vector_mask;

  template<class T, usize N, class Env>
  using vector_mask = basic_vector_mask<detail::mask_element_t<T>, N, Env>;

  template<class T, usize N, class Env>
  struct basic_bit_mask;

  template<class T, usize N, class Env>
  using bit_mask = basic_bit_mask<detail::mask_element_t<T>, N, Env>;

}  // namespace lps::doubling
