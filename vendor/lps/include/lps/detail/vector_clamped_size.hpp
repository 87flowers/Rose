#pragma once

#include <algorithm>
#include <lps/stdint.hpp>

namespace lps::detail {

  // To fill up a 128-bit vector.
  template<class T>
  constexpr usize min_size = 16 * sizeof(u8) / sizeof(T);

  template<class T, usize N>
  constexpr usize clamped_size = N < min_size<T> ? min_size<T> : N;

  template<class Env, class T, usize N>
  using vector_clamped_size = typename Env::template vector<T, clamped_size<T, N>>;

}  // namespace lps::detail
