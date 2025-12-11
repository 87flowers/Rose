#pragma once

#include <concepts>
#include <fmt/format.h>
#include <source_location>

namespace rose::internal {
  auto die(std::source_location location, std::string_view expr) -> void;
  auto vdie(std::source_location location, std::string_view expr, std::string_view fmt, fmt::format_args args) -> void;

  template<typename... Args>
  inline auto die(std::source_location location, std::string_view expr, std::string_view fmt, Args&&... args) -> void {
    vdie(location, expr, fmt, fmt::make_format_args(args...));
  }

  auto vdbg(std::string_view fmt, fmt::format_args args) -> void;

  template<typename... Args>
  auto dbg(std::string_view fmt, Args&&... args) -> void {
    vdbg(fmt, fmt::make_format_args(args...));
  }
}  // namespace rose::internal

#if ROSE_NO_ASSERTS
#define rose_assert(expr, ...)
#else
#define rose_assert(expr, ...)                                                                                                                       \
  if (!(expr)) [[unlikely]]                                                                                                                          \
    rose::internal::die(std::source_location::current(), #expr __VA_OPT__(, __VA_ARGS__));
#endif

#define rose_dbg(...) rose::internal::dbg(__VA_ARGS__);

namespace rose {
  template<std::integral Dest>
  inline constexpr auto narrow_cast(std::integral auto src) -> Dest {
    static_assert(sizeof(Dest) < sizeof(decltype(src)));
    rose_assert(static_cast<Dest>(src) == src);
    return static_cast<Dest>(src);
  }
}  // namespace rose
