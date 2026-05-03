#pragma once

#include "rose/bitboard.hpp"
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
    std::array<PieceMask, 64> m_at;
    Bitboard m_pinned;

    enum class Mode {
      all,
      noisy,
      quiet,
    };

    template<MoveFlags mf>
    auto write_moves(MoveList& moves, const std::array<PieceMask, 64>& at, u16x16 srcs, Bitboard bb, PieceMask pm) -> void;
    template<MoveFlags mf>
    auto write_moves(MoveList& moves, Square from, Bitboard to_bb) -> void;
    auto write_cap_promo(MoveList& moves, const std::array<PieceMask, 64>& at, Bitboard bb, PieceMask pm) -> void;

    template<bool in_check, Mode mode>
    auto generate_moves_to(MoveList& moves, Square king_sq, Bitboard valid_destinations, PieceType one_checker) -> void;
    template<Mode mode>
    auto generate_king_moves_with_checkers(MoveList& moves, Square king_sq, PieceMask checkers) -> void;

    template<Mode mode>
    auto generate_moves_no_checkers(MoveList& moves, Square king_sq) -> void;
    template<Mode mode>
    auto generate_moves_one_checker(MoveList& moves, Square king_sq, PieceMask checkers) -> void;
    template<Mode mode>
    auto generate_moves_two_checkers(MoveList& moves, Square king_sq, PieceMask checkers) -> void;

    template<Mode mode>
    auto generate_moves(MoveList& moves) -> void;

  public:
    explicit MoveGen(const Position& position);

    auto prepare() -> void;

    auto generate_all(MoveList& moves) -> void {
      generate_moves<Mode::all>(moves);
    }

    auto generate_noisy(MoveList& moves) -> void {
      generate_moves<Mode::noisy>(moves);
    }

    auto generate_quiet(MoveList& moves) -> void {
      generate_moves<Mode::quiet>(moves);
    }
  };

  inline auto generate_all_moves(const Position& position) -> MoveList {
    MoveList moves;
    MoveGen movegen {position};
    movegen.prepare();
    movegen.generate_all(moves);
    return moves;
  }

}  // namespace rose
