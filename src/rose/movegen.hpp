#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/position.hpp"
#include "rose/util/static_vector.hpp"

namespace rose {

  constexpr usize max_legal_moves = 256;

  struct MoveList : StaticVector<Move, max_legal_moves> {
    template<typename M, typename T>
    auto write(M mask, T v) -> void;
    template<typename M, typename T>
    auto write2(M mask, T v) -> void;
    template<typename M, typename T>
    auto write4(M mask, T v) -> void;
  };

  struct MoveGen {
  private:
    const Position& m_position;

    template<MoveFlags mf>
    auto write_moves(MoveList& moves, const std::array<PieceMask, 64>& at, u16x16 srcs, Bitboard bb, PieceMask pm) -> void;
    auto write_cap_promo(MoveList& moves, const std::array<PieceMask, 64>& at, Bitboard bb, PieceMask pm) -> void;

    auto generate_moves_no_checkers(MoveList& moves, Square king_sq) -> void;
    auto generate_moves_one_checker(MoveList& moves, Square king_sq, PieceMask checkers) -> void;
    auto generate_moves_two_checkers(MoveList& moves, Square king_sq, PieceMask checkers) -> void;

  public:
    explicit MoveGen(const Position& position);

    auto generate_moves(MoveList& moves) -> void;
  };

}  // namespace rose
