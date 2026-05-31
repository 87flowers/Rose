#include "rose/common.hpp"
#include "rose/tool/datagen/datagen.hpp"
#include "rose/util/string.hpp"

#include <fmt/format.h>

auto main(int argc, char** argv) -> int {
  if (argc != 2) {
    fmt::print("Usage: {} <thread count>\n", argv[0]);
    return 1;
  }

  const auto thread_count = rose::parse_usize(argv[1]);
  if (!thread_count) {
    fmt::print("Invalid thread count\n");
    return 1;
  }

  rose::tool::datagen::run(*thread_count);
  return 0;
}
