#pragma once

#include "rose/bitboard.hpp"
#include "rose/common.hpp"
#include "rose/move.hpp"

#include <array>
#include <utility>

namespace rose::pawns {

  struct Bitboards {
    Bitboard normal_move;
    Bitboard double_move;
  };

  constexpr auto pawn_destination_empty(Color stm, Bitboard empty, Bitboard valid_destinations) -> Bitboards {
    switch (stm.raw) {
    case Color::white:
      return {(empty & valid_destinations) >> 8, (empty >> 8) & ((empty & valid_destinations) >> 16)};
    case Color::black:
      return {(empty & valid_destinations) << 8, (empty << 8) & ((empty & valid_destinations) << 16)};
    }
    std::unreachable();
  }

  struct Shifts {
    Bitboard promo_zone;
    Bitboard non_promo_dest;
    int second_rank_shift;
    int promotable_shift;
  };

  constexpr auto pawn_shifts(Color stm) -> Shifts {
    switch (stm.raw) {
    case Color::white:
      return {Bitboard {0xff00000000000000}, Bitboard {0x00ffffffffff0000}, 8, 48};
    case Color::black:
      return {Bitboard {0x00000000000000ff}, Bitboard {0x0000ffffffffff00}, 48, 8};
    }
    std::unreachable();
  }

  struct Moves {
    u16x32 normal_moves;
    u16x16 double_moves;
    u16x32 promotions;
  };

  auto pawn_moves(Color stm) -> Moves {
    constexpr std::array<u16x32, 2> normal_moves = [] consteval {
      std::array<std::array<u16, 32>, 2> normal_moves;
      for (u8 i = 16; i < 48; i++) {
        normal_moves[0][i - 16] = Move::make(Square {i}, Square {narrow_cast<u8>(i + 8)}, MoveFlags::normal).raw;
        normal_moves[1][i - 16] = Move::make(Square {i}, Square {narrow_cast<u8>(i - 8)}, MoveFlags::normal).raw;
      }
      return std::array<u16x32, 2> {u16x32 {normal_moves[0]}, u16x32 {normal_moves[1]}};
    }();
    constexpr std::array<u16x16, 2> double_moves = [] consteval {
      std::array<std::array<u16, 16>, 2> double_moves;
      for (u8 i = 0; i < 8; i++) {
        const u8 wsrc = 8 + i;
        double_moves[0][i + 0] = Move::make(Square {wsrc}, Square {narrow_cast<u8>(wsrc + 8)}, MoveFlags::normal).raw;
        double_moves[0][i + 8] = Move::make(Square {wsrc}, Square {narrow_cast<u8>(wsrc + 16)}, MoveFlags::double_push).raw;
        const u8 bsrc = 48 + i;
        double_moves[1][i + 0] = Move::make(Square {bsrc}, Square {narrow_cast<u8>(bsrc - 8)}, MoveFlags::normal).raw;
        double_moves[1][i + 8] = Move::make(Square {bsrc}, Square {narrow_cast<u8>(bsrc - 16)}, MoveFlags::double_push).raw;
      }
      return std::array<u16x16, 2> {u16x16 {double_moves[0]}, u16x16 {double_moves[1]}};
    }();
    constexpr std::array<u16x32, 2> promotions = [] consteval {
      std::array<std::array<u16, 32>, 2> promotions;
      for (u8 i = 0; i < 8; i++) {
        const u8 wsrc = 48 + i;
        promotions[0][i * 4 + 0] = Move::make(Square {wsrc}, Square {narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_q).raw;
        promotions[0][i * 4 + 1] = Move::make(Square {wsrc}, Square {narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_n).raw;
        promotions[0][i * 4 + 2] = Move::make(Square {wsrc}, Square {narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_r).raw;
        promotions[0][i * 4 + 3] = Move::make(Square {wsrc}, Square {narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_b).raw;
        const u8 bsrc = 8 + i;
        promotions[1][i * 4 + 0] = Move::make(Square {bsrc}, Square {narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_q).raw;
        promotions[1][i * 4 + 1] = Move::make(Square {bsrc}, Square {narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_n).raw;
        promotions[1][i * 4 + 2] = Move::make(Square {bsrc}, Square {narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_r).raw;
        promotions[1][i * 4 + 3] = Move::make(Square {bsrc}, Square {narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_b).raw;
      }
      return std::array<u16x32, 2> {u16x32 {promotions[0]}, u16x32 {promotions[1]}};
    }();

    const int j = stm.to_index();
    return {normal_moves[j], double_moves[j], promotions[j]};
  }

}  // namespace rose::pawns
