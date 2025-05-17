#pragma once

#include <array>
#include <tuple>

#include "rose/common.h"
#include "rose/move.h"
#include "rose/position.h"
#include "rose/util/static_vector.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  struct MoveList : StaticVector<Move, max_legal_moves> {
    template <typename T> forceinline auto write(typename T::Mask16 mask, T v) -> void;
    template <typename T> forceinline auto write2(typename T::Mask32 mask, T v) -> void;
    template <typename T> forceinline auto write4(typename T::Mask64 mask, T v) -> void;
    forceinline auto write4(u64 moves) -> void;
  };

  struct PrecompMoveGenInfo {
    std::array<u64, 2> aside_rook;
    std::array<u64, 2> aside_king;
    std::array<u64, 2> hside_rook;
    std::array<u64, 2> hside_king;

    PrecompMoveGenInfo() : PrecompMoveGenInfo(Position::startpos()) {}
    explicit PrecompMoveGenInfo(const Position &position);
  };

  struct MoveLists {
    MoveList &unprotected_noisy;
    MoveList &protected_noisy;
    MoveList &unprotected_quiet;
    MoveList &protected_quiet;
  };

  struct MoveGen {
  private:
    const Position &m_position;
    const PrecompMoveGenInfo &m_precomp_info;

    auto calculatePinInfo() -> std::tuple<std::array<v512, 2>, u64>;

    auto generateSubsetNorm(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void;
    auto generateSubsetCaps(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void;
    auto generateSubsetPCap(MoveList &moves, const Wordboard &attack_table, u64 bitboard, u16 piecemask) -> void;

    template <bool king_moves>
    forceinline auto generateMovesTo(const MoveLists &moves, Square king_sq, u64 valid_destinations, PieceType checker_ptype) -> void;
    auto generateMovesNoCheckers(const MoveLists &moves, Square king_sq) -> void;
    auto generateMovesOneChecker(const MoveLists &moves, Square king_sq, u16 checkers) -> void;
    auto generateMovesTwoCheckers(const MoveLists &moves, Square king_sq, u16 checkers) -> void;

  public:
    MoveGen(const Position &position, const PrecompMoveGenInfo &precomp_info) : m_position(position), m_precomp_info(precomp_info) {}

    auto generateMoves(const MoveLists &moves) -> void;
    auto generateMoves(MoveList &moves) -> void { generateMoves({moves, moves, moves, moves}); }
  };

} // namespace rose
