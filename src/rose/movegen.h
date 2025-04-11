#pragma once

#include "rose/common.h"
#include "rose/move.h"
#include "rose/position.h"
#include "rose/util/static_vector.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  struct MoveList : StaticVector<Move, max_legal_moves> {
    template <typename T> auto write(typename T::Mask16 mask, T v) -> void;
  };

  struct MoveGen {
  private:
    const Position &m_position;
    Square m_king_sq;

    // Pin info
    v512 m_king_ray_coords;
    v512 m_king_ray_places;
    u64 m_king_ray_valid;
    u64 m_pin_raymasks;
    u64 m_pinned;          // mask for m_king_ray_coords
    u64 m_pinned_friendly; // mask for m_king_ray_coords
    u16 m_pinned_piece_mask;

    auto calculatePinInfo() -> void;

  public:
    explicit MoveGen(const Position &position) : m_position(position), m_king_sq(position.kingSq(position.activeColor())) {}

    auto generateMoves(MoveList &moves) -> void;
  };

} // namespace rose
