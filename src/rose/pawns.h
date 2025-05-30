#pragma once

#include <array>
#include <utility>

#include "rose/common.h"
#include "rose/move.h"
#include "rose/util/types.h"

namespace rose::pawns {

  struct Bitboards {
    u64 normal_move;
    u64 double_move;
  };

  forceinline constexpr auto pawnDestinationEmpty(Color perspective, u64 empty, u64 valid_destinations) -> Bitboards {
    switch (perspective.raw) {
    case Color::white:
      return {(empty & valid_destinations) >> 8, (empty >> 8) & ((empty & valid_destinations) >> 16)};
    case Color::black:
      return {(empty & valid_destinations) << 8, (empty << 8) & ((empty & valid_destinations) << 16)};
    }
    std::unreachable();
  }

  struct Shifts {
    u64 promo_zone;
    u64 non_promo_dest;
    int second_rank_shift;
    int promotable_shift;
  };

  forceinline constexpr auto pawnShifts(Color perspective) -> Shifts {
    switch (perspective.raw) {
    case Color::white:
      return {0xff00000000000000, 0x00ffffffffff0000, 8, 48};
    case Color::black:
      return {0x00000000000000ff, 0x0000ffffffffff00, 48, 8};
    }
    std::unreachable();
  }

  struct Moves {
    v512 normal_moves;
    v256 double_moves;
    v512 promotions;
  };

  forceinline auto pawnMoves(Color perspective) -> Moves {
    constexpr std::array<v512, 2> normal_moves = [] {
      std::array<std::array<u16, 32>, 2> normal_moves;
      for (u8 i = 16; i < 48; i++) {
        normal_moves[0][i - 16] = Move::make(Square{i}, Square{narrow_cast<u8>(i + 8)}, MoveFlags::normal).raw;
        normal_moves[1][i - 16] = Move::make(Square{i}, Square{narrow_cast<u8>(i - 8)}, MoveFlags::normal).raw;
      }
      return std::array<v512, 2>{v512{normal_moves[0]}, v512{normal_moves[1]}};
    }();
    constexpr std::array<v256, 2> double_moves = [] {
      std::array<std::array<u16, 16>, 2> double_moves;
      for (u8 i = 0; i < 8; i++) {
        const u8 wsrc = 8 + i;
        double_moves[0][i + 0] = Move::make(Square{wsrc}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::normal).raw;
        double_moves[0][i + 8] = Move::make(Square{wsrc}, Square{narrow_cast<u8>(wsrc + 16)}, MoveFlags::double_push).raw;
        const u8 bsrc = 48 + i;
        double_moves[1][i + 0] = Move::make(Square{bsrc}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::normal).raw;
        double_moves[1][i + 8] = Move::make(Square{bsrc}, Square{narrow_cast<u8>(bsrc - 16)}, MoveFlags::double_push).raw;
      }
      return std::array<v256, 2>{v256{double_moves[0]}, v256{double_moves[1]}};
    }();
    constexpr std::array<v512, 2> promotions = [] {
      std::array<std::array<u16, 32>, 2> promotions;
      for (u8 i = 0; i < 8; i++) {
        const u8 wsrc = 48 + i;
        promotions[0][i * 4 + 0] = Move::make(Square{wsrc}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_q).raw;
        promotions[0][i * 4 + 1] = Move::make(Square{wsrc}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_n).raw;
        promotions[0][i * 4 + 2] = Move::make(Square{wsrc}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_r).raw;
        promotions[0][i * 4 + 3] = Move::make(Square{wsrc}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_b).raw;
        const u8 bsrc = 8 + i;
        promotions[1][i * 4 + 0] = Move::make(Square{bsrc}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_q).raw;
        promotions[1][i * 4 + 1] = Move::make(Square{bsrc}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_n).raw;
        promotions[1][i * 4 + 2] = Move::make(Square{bsrc}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_r).raw;
        promotions[1][i * 4 + 3] = Move::make(Square{bsrc}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_b).raw;
      }
      return std::array<v512, 2>{v512{promotions[0]}, v512{promotions[1]}};
    }();

    const int j = perspective.toIndex();
    return {normal_moves[j], double_moves[j], promotions[j]};
  }

} // namespace rose::pawns
