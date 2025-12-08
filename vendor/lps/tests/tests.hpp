#pragma once

#include <cstdlib>
#include <print>
#include <source_location>

#define REQUIRE(x)                                                             \
  do {                                                                         \
    if (!(x)) {                                                                \
      std::print("Assertion failed at {}: {}",                                 \
                 std::source_location::current(), #x);                         \
      std::exit(1);                                                            \
    }                                                                          \
  } while (false)
