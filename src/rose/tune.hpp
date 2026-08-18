#pragma once

#include "rose/common.hpp"

#include <string_view>

namespace rose::tune {

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
  rose_define_tunable_marker(_zt);

  // clang-format off
#define for_each_rose_tunable(x) \
  x(f64, search03, 0.5, 0, 1.0, 0.05, 0.002) \
  x(f64, search07, 0.667434450591335, 0, 1.3348689011827, 0.066743445059134, 0.002) \
  x(i32, search00, 28, 0, 56, 2.8, 0.002) \
  x(f64, search06, 0.04828987665300888, 0, 0.096579753306018, 0.0048289876653009, 0.002) \
  x(f64, search05, 0.592028611604442, 0, 1.1840572232089, 0.059202861160444, 0.002) \
  x(f64, search01, 2.0, 0, 4.0, 0.2, 0.002) \
  x(f64, search02, 1.5, 0, 3.0, 0.15, 0.002) \
  x(f64, search04, 0.05116902193882232, 0, 0.10233804387764, 0.0051169021938822, 0.002) \
  /* End of Tunables */
  // clang-format on

#define rose_define_tunable_template(T, n, v, ...) inline T n = v;
  for_each_rose_tunable(rose_define_tunable_template);
#undef rose_define_tunable_template

  auto uci_print_options() -> void;
  auto uci_parse_option(std::string_view name, std::string_view value) -> bool;

}  // namespace rose::tune
