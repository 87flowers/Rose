#pragma once

#include "lps/stdint.hpp"

namespace lps::detail {

  template<class T>
  struct mask_element;

  template<>
  struct mask_element<i8> {
    using type = u8;
  };

  template<>
  struct mask_element<i16> {
    using type = u16;
  };

  template<>
  struct mask_element<i32> {
    using type = u32;
  };

  template<>
  struct mask_element<i64> {
    using type = u64;
  };

  template<>
  struct mask_element<u8> {
    using type = u8;
  };

  template<>
  struct mask_element<u16> {
    using type = u16;
  };

  template<>
  struct mask_element<u32> {
    using type = u32;
  };

  template<>
  struct mask_element<u64> {
    using type = u64;
  };

  template<>
  struct mask_element<float> {
    static_assert(sizeof(u32) == sizeof(float));
    using type = u32;
  };

  template<>
  struct mask_element<double> {
    static_assert(sizeof(u64) == sizeof(double));
    using type = u64;
  };

  template<class T>
  using mask_element_t = typename mask_element<T>::type;

}  // namespace lps::detail
