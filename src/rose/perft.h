#pragma once

#include "rose/util/types.h"

namespace rose {
  struct Position;
} // namespace rose

namespace rose::perft {

  auto value(const Position &position, usize depth) -> usize;
  auto run(const Position &position, usize depth) -> void;

} // namespace rose::perft
