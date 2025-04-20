#include <cstdio>
#include <iostream>
#include <print>
#include <ranges>

#include "rose/game.h"
#include "rose/uci.h"
#include "rose/util/types.h"

auto main(int argc, char *argv[]) -> int {
  std::print("# 🌹 Rose {}\n", ROSE_VERSION);
  std::fflush(stdout);

  rose::Game game;
  game.reset();

  if (argc > 1) {
    for (rose::usize i : std::views::iota(1, argc)) {
      rose::uciParseCommand(game, argv[i]);
    }
    return 0;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    rose::uciParseCommand(game, line);
  }
  return 0;
}
