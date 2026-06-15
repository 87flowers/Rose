#pragma once

#include "lps/stdint.hpp"

namespace lps::detail {

  template<class T>
  struct double_element_size;

  template<>
  struct double_element_size<i8> {
    using type = i16;
  };

  template<>
  struct double_element_size<i16> {
    using type = i32;
  };

  template<>
  struct double_element_size<i32> {
    using type = i64;
  };

  template<>
  struct double_element_size<u8> {
    using type = u16;
  };

  template<>
  struct double_element_size<u16> {
    using type = u32;
  };

  template<>
  struct double_element_size<u32> {
    using type = u64;
  };

  template<class T>
  using double_element_size_t = typename double_element_size<T>::type;

}  // namespace lps::detail
