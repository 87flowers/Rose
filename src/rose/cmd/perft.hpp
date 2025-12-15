#pragma once

#include "rose/common.hpp"
#include "rose/position.hpp"

namespace rose::perft {

  auto value(const Position& position, usize depth) -> u64;
  auto run(const Position& position, usize depth, bool bulk) -> void;

}  // namespace rose::perft
