#pragma once

#include <cstdlib>
#include <print>
#include <source_location>

#define REQUIRE(x, msg, ...)                                                                                                                         \
  do {                                                                                                                                               \
    if (!(x)) {                                                                                                                                      \
      const auto loc = std::source_location::current();                                                                                              \
      std::print("Assertion failed at {}:{}:{}: {}\n", loc.file_name(), loc.line(), loc.column(), #x);                                               \
      std::print(msg "\n", __VA_ARGS__);                                                                                                             \
      std::exit(1);                                                                                                                                  \
    }                                                                                                                                                \
  } while (false)
