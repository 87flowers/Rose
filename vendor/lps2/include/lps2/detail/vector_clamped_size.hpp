#pragma once

#include <lps2/stdint.hpp>

namespace lps2 {
  template<class T, usize N>
  struct vector;
}  // namespace lps2

namespace lps2::detail {

  // To fill up a 128-bit vector.
  template<class T>
  constexpr usize min_size = 16 * sizeof(u8) / sizeof(T);

  template<class T, usize N>
  constexpr usize clamped_size = N < min_size<T> ? min_size<T> : N;

  template<class T, usize N>
  using vector_clamped_size = vector<T, clamped_size<T, N>>;

}  // namespace lps2::detail
