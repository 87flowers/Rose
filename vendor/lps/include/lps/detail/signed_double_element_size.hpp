#pragma once

#include "lps/stdint.hpp"

namespace lps::detail {

  template<class T>
  struct signed_double_element_size {};

  template<>
  struct signed_double_element_size<i8> {
    using type = i16;
  };

  template<>
  struct signed_double_element_size<i16> {
    using type = i32;
  };

  template<>
  struct signed_double_element_size<i32> {
    using type = i64;
  };

  template<>
  struct signed_double_element_size<u8> {
    using type = u16;
  };

  template<>
  struct signed_double_element_size<u16> {
    using type = u32;
  };

  template<>
  struct signed_double_element_size<u32> {
    using type = u64;
  };

  template<class T>
  using signed_double_element_size_t = typename signed_double_element_size<T>::type;

}  // namespace lps::detail
