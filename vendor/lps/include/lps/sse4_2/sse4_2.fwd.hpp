#pragma once

#include "lps/detail/mask_element.hpp"
#include "lps/stdint.hpp"

namespace lps::sse4_2 {

  template<class T, usize N, class Env>
  struct vector;

  template<class T, usize N, class Env>
  struct basic_vector_mask;

  template<class T, usize N, class Env>
  using vector_mask = basic_vector_mask<detail::mask_element_t<T>, N, Env>;

}  // namespace lps::sse4_2
