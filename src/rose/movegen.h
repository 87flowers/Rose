#pragma once

#include "rose/common.h"
#include "rose/move.h"
#include "rose/util/static_vector.h"

namespace rose {
  struct Position;
} // namespace rose

namespace rose::movegen {

  struct MoveList : StaticVector<Move, max_legal_moves> {
    template <typename T> auto write(typename T::Mask16 mask, T v) -> void;
  };

  auto generateMoves(MoveList &moves, const Position &position) -> void;

} // namespace rose::movegen
