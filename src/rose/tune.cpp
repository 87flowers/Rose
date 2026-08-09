#include "rose/tune.hpp"

#include "rose/util/string.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <type_traits>

namespace rose::tune {

  auto uci_print_options() -> void {
#define rose_print_tuneable_template(T, n, v, min, max, ...)                                                                                         \
  fmt::print("option name {} type {} default {} min {} max {}\n", #n, std::is_same_v<T, f64> ? "string" : "spin", v, min, max);
    for_each_rose_tunable(rose_print_tuneable_template)
#undef rose_print_tuneable_template
  }

  auto uci_parse_option(std::string_view name, std::string_view value_str) -> bool {
#define rose_parse_tunable_template(T, n, v, min, max, ...)                                                                                          \
  if (name == #n) {                                                                                                                                  \
    if (auto value = parse_##T(value_str)) {                                                                                                         \
      n = std::clamp<T>(*value, min, max);                                                                                                           \
      return true;                                                                                                                                   \
    } else {                                                                                                                                         \
      return false;                                                                                                                                  \
    }                                                                                                                                                \
  }
    for_each_rose_tunable(rose_parse_tunable_template);
#undef rose_parse_tunable_template
    return false;
  }

}  // namespace rose::tune
