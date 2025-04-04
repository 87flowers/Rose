#pragma once

#include "rose/common.h"
#include "rose/move.h"
#include "rose/util/static_vector.h"

namespace rose {
  struct Position;
} // namespace rose

namespace rose::movegen {

  using MoveList = StaticVector<Move, max_legal_moves>;

  auto generateMoves(MoveList &moves, const Position &position) -> void;

} // namespace rose::movegen
