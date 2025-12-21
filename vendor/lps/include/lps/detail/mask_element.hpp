#pragma once

#include "lps/stdint.hpp"

namespace lps::detail {

  template<class T>
  struct mask_element;

  template<>
  struct mask_element<i8> {
    using type = i8;
  };

  template<>
  struct mask_element<i16> {
    using type = i16;
  };

  template<>
  struct mask_element<i32> {
    using type = i32;
  };

  template<>
  struct mask_element<i64> {
    using type = i64;
  };

  template<>
  struct mask_element<u8> {
    using type = i8;
  };

  template<>
  struct mask_element<u16> {
    using type = i16;
  };

  template<>
  struct mask_element<u32> {
    using type = i32;
  };

  template<>
  struct mask_element<u64> {
    using type = i64;
  };

  template<>
  struct mask_element<float> {
    static_assert(sizeof(i32) == sizeof(float));
    using type = i32;
  };

  template<>
  struct mask_element<double> {
    static_assert(sizeof(i64) == sizeof(double));
    using type = i64;
  };

  template<class T>
  using mask_element_t = typename mask_element<T>::type;

}  // namespace lps::detail
