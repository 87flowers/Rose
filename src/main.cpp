#include "rose/interface.hpp"
#include "rose/version.hpp"

#include <fmt/format.h>
#include <iostream>
#include <ranges>

auto main(int argc, char** argv) -> int {
  fmt::print("# 🌹 Rose {}\n", rose::version::to_string());

  rose::Interface engine;

  if (argc > 1) {
    for (rose::usize i : std::views::iota(1, argc)) {
      engine.parse_command(argv[i]);
    }
    return 0;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    engine.parse_command(line);
  }
  return 0;
}
