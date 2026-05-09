#pragma once

#include "rose/common.hpp"

#include <array>

namespace rose {
  namespace internal {
    template<typename T, usize n0, usize... ns>
    struct multi_array {
      using type = std::array<typename multi_array<T, ns...>::type, n0>;
    };

    template<typename T, usize n0>
    struct multi_array<T, n0> {
      using type = std::array<T, n0>;
    };
  }  // namespace internal

  template<typename T, usize... ns>
  using multi_array = typename internal::multi_array<T, ns...>::type;
}  // namespace rose
