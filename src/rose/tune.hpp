#pragma once

#include "rose/common.hpp"

#include <string_view>

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
