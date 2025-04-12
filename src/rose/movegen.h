#pragma once

#include <array>

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

  struct PrecompMoveGenInfo {
    std::array<u64, 2> aside_rook;
    std::array<u64, 2> aside_king;
    std::array<u64, 2> hside_rook;
    std::array<u64, 2> hside_king;

    explicit PrecompMoveGenInfo(const Position &position);
  };

  struct MoveGen {
  private:
    const Position &m_position;
    Square m_king_sq;
    const PrecompMoveGenInfo &m_precomp_info;

    // Pin info
    v512 m_king_ray_coords;
    v512 m_king_ray_places;
    u64 m_king_ray_valid;
    u64 m_pin_raymasks;
    u64 m_pinned;          // mask for m_king_ray_coords
    u64 m_pinned_friendly; // mask for m_king_ray_coords
    u16 m_pinned_piece_mask;

    auto calculatePinInfo() -> void;

    auto generateSubsetNorm(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void;
    auto generateSubsetCaps(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void;
    auto generateSubsetPCap(MoveList &moves, const Wordboard &attack_table, u64 bitboard, u16 piecemask) -> void;

    auto generateMovesNoCheckers(MoveList &moves, const Position &position, Square king_sq) -> void;

  public:
    MoveGen(const Position &position, const PrecompMoveGenInfo &precomp_info)
        : m_position(position), m_king_sq(position.kingSq(position.activeColor())), m_precomp_info(precomp_info) {}

    auto generateMoves(MoveList &moves) -> void;
  };

} // namespace rose
