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
    template <typename T> auto write2(typename T::Mask32 mask, T v) -> void;
    template <typename T> auto write4(typename T::Mask64 mask, T v) -> void;
    auto write4(u64 moves) -> void;
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

    auto generateMoveSubset(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void;
    auto generatePromoMoveSubset(MoveList &moves, const Wordboard &attack_table, u64 bitboard, u16 piecemask) -> void;

    auto generateMovesNoCheckers(MoveList &moves, const Position &position, Square king_sq) -> void;

  public:
    explicit MoveGen(const Position &position) : m_position(position), m_king_sq(position.kingSq(position.activeColor())) {}

    auto generateMoves(MoveList &moves) -> void;
  };

} // namespace rose
