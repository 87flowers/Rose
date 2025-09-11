#pragma once

#include "rose/move.h"
#include "rose/position.h"

namespace rose::see {

  auto see(const Position &position, Move move, i32 threshold) -> bool;

} // namespace rose::see
