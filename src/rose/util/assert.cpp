#include "rose/util/assert.h"

#include <cstdio>
#include <print>
#include <utility>

namespace rose::internal {
  auto die(std::source_location loc, std::string_view expr) -> void {
    std::print(stderr, "{}:{}:{} `{}`: assertion failed: {}\n", loc.file_name(), loc.line(), loc.column(), loc.function_name(), expr);
    std::terminate();
  }

  auto vdie(std::source_location loc, std::string_view expr, std::string_view fmt, std::format_args args) -> void {
    std::print(stderr, "{}:{}:{} `{}`: assertion failed: {}\n", loc.file_name(), loc.line(), loc.column(), loc.function_name(), expr);
    std::vprint_unicode(stderr, fmt, args);
    std::print(stderr, "\n");
    std::terminate();
  }

  auto vdbg(std::string_view fmt, std::format_args args) -> void { std::vprint_unicode(stderr, fmt, args); }
} // namespace rose::internal
