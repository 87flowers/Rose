#pragma once

#include "rose/common.hpp"

#include <string_view>

namespace rose {

  // clang-format off
#define rose_define_tunable_marker(postfix)                                                                                                          \
  constexpr auto operator""##postfix(unsigned long long value) -> i32 {                                                                              \
    return static_cast<i32>(value);                                                                                                                  \
  }                                                                                                                                                  \
                                                                                                                                                     \
  constexpr auto operator""##postfix(long double value) -> f64 {                                                                                     \
    return static_cast<f64>(value);                                                                                                                  \
  }
  // clang-format on

  rose_define_tunable_marker(_z);

}  // namespace rose

namespace rose::tune {

  // clang-format off
#define for_each_rose_tunable(x) \
  /* End of Tunables */
  // clang-format on

#define rose_define_tunable_template(T, n, v, ...) inline T n = v;
  for_each_rose_tunable(rose_define_tunable_template);
#undef rose_define_tunable_template

  auto uci_print_options() -> void;
  auto uci_parse_option(std::string_view name, std::string_view value) -> bool;

}  // namespace rose::tune
