#include <cstdio>
#include <iostream>
#include <print>
#include <ranges>
#include <unistd.h>

#include "rose/engine.h"
#include "rose/game.h"
#include "rose/uci.h"
#include "rose/util/types.h"

auto main(int argc, char *argv[]) -> int {
  if (isatty(STDOUT_FILENO)) {
    std::print("# 🌹 Rose {}-{}\n", ROSE_VERSION, ROSE_GIT_COMMIT_DESC);
    std::fflush(stdout);
  }

  rose::Engine engine;
  rose::Game game;

  engine.reset();
  game.reset();

  if (argc > 1) {
    for (rose::usize i : std::views::iota(1, argc)) {
      rose::uciParseCommand(engine, game, argv[i]);
    }
    return 0;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    rose::uciParseCommand(engine, game, line);
  }
  return 0;
}
