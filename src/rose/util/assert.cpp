#include "rose/util/assert.hpp"

#include <cstdio>
#include <fmt/format.h>
#include <utility>

namespace rose::internal {
  auto die(std::source_location loc, std::string_view expr) -> void {
    fmt::print(stderr, "{}:{}:{} `{}`: assertion failed: {}\n", loc.file_name(), loc.line(), loc.column(), loc.function_name(), expr);
    std::terminate();
  }

  auto vdie(std::source_location loc, std::string_view expr, std::string_view fmt, fmt::format_args args) -> void {
    fmt::print(stderr, "{}:{}:{} `{}`: assertion failed: {}\n", loc.file_name(), loc.line(), loc.column(), loc.function_name(), expr);
    fmt::vprint(stderr, fmt, args);
    fmt::print(stderr, "\n");
    std::terminate();
  }

  auto vdbg(std::string_view fmt, fmt::format_args args) -> void {
    fmt::vprint(stderr, fmt, args);
  }
}  // namespace rose::internal
