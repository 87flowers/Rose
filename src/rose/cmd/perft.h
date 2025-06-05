#pragma once

#include "rose/util/types.h"

namespace rose {
  struct Position;
} // namespace rose

namespace rose::perft {

  auto value(const Position &position, usize depth) -> u64;
  auto run(const Position &position, usize depth, bool bulk) -> void;

} // namespace rose::perft
