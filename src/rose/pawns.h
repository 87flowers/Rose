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

  forceinline auto promotions(Color perspective, v512 ids) -> v512 {
    static constexpr std::array<v512, 2> lut = [] {
      std::array<std::array<u16, 32>, 2> lut;
      for (u8 i = 0; i < 8; i++) {
        const u8 wsrc = 48 + i;
        lut[0][i * 4 + 0] = Move::make(PieceId{0}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_q).raw;
        lut[0][i * 4 + 1] = Move::make(PieceId{0}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_n).raw;
        lut[0][i * 4 + 2] = Move::make(PieceId{0}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_r).raw;
        lut[0][i * 4 + 3] = Move::make(PieceId{0}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::promo_b).raw;
        const u8 bsrc = 8 + i;
        lut[1][i * 4 + 0] = Move::make(PieceId{0}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_q).raw;
        lut[1][i * 4 + 1] = Move::make(PieceId{0}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_n).raw;
        lut[1][i * 4 + 2] = Move::make(PieceId{0}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_r).raw;
        lut[1][i * 4 + 3] = Move::make(PieceId{0}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::promo_b).raw;
      }
      return std::array<v512, 2>{v512{lut[0]}, v512{lut[1]}};
    }();

    switch (perspective.raw) {
    case Color::white:
      return vec::permute8_mz(0x5555555555555555,
                              v512{std::array<u8, 64>{
                                  48, 0, 48, 0, 48, 0, 48, 0, 49, 0, 49, 0, 49, 0, 49, 0, 50, 0, 50, 0, 50, 0, 50, 0, 51, 0, 51, 0, 51, 0, 51, 0,
                                  52, 0, 52, 0, 52, 0, 52, 0, 53, 0, 53, 0, 53, 0, 53, 0, 54, 0, 54, 0, 54, 0, 54, 0, 55, 0, 55, 0, 55, 0, 55, 0,
                              }},
                              ids) |
             lut[0];
    case Color::black:
      return vec::permute8_mz(0x5555555555555555, v512{std::array<u8, 64>{8,  0, 8,  0, 8,  0, 8,  0, 9,  0, 9,  0, 9,  0, 9,  0, 10, 0, 10, 0, 10, 0,
                                                                          10, 0, 11, 0, 11, 0, 11, 0, 11, 0, 12, 0, 12, 0, 12, 0, 12, 0, 13, 0, 13, 0,
                                                                          13, 0, 13, 0, 14, 0, 14, 0, 14, 0, 14, 0, 15, 0, 15, 0, 15, 0, 15, 0}},
                              ids) |
             lut[1];
    }
    std::unreachable();
  }

  forceinline auto normalMoves(Color perspective, v512 ids) -> v512 {
    static constexpr std::array<v512, 2> lut = [] {
      std::array<std::array<u16, 32>, 2> lut;
      for (u8 i = 16; i < 48; i++) {
        lut[0][i - 16] = Move::make(PieceId{0}, Square{narrow_cast<u8>(i + 8)}, MoveFlags::normal).raw;
        lut[1][i - 16] = Move::make(PieceId{0}, Square{narrow_cast<u8>(i - 8)}, MoveFlags::normal).raw;
      }
      return std::array<v512, 2>{v512{lut[0]}, v512{lut[1]}};
    }();

    return vec::permute8_mz(0x5555555555555555,
                            v512{std::array<u8, 64>{
                                16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0, 22, 0, 23, 0, 24, 0, 25, 0, 26, 0, 27, 0, 28, 0, 29, 0, 30, 0, 31, 0,
                                32, 0, 33, 0, 34, 0, 35, 0, 36, 0, 37, 0, 38, 0, 39, 0, 40, 0, 41, 0, 42, 0, 43, 0, 44, 0, 45, 0, 46, 0, 47, 0,
                            }},
                            ids) |
           lut[perspective.toIndex()];
    std::unreachable();
  }

  forceinline auto doubleMoves(Color perspective, v512 ids) -> v256 {
    static constexpr std::array<v256, 2> lut = [] {
      std::array<std::array<u16, 16>, 2> lut;
      for (u8 i = 0; i < 8; i++) {
        const u8 wsrc = 8 + i;
        lut[0][i + 0] = Move::make(PieceId{0}, Square{narrow_cast<u8>(wsrc + 8)}, MoveFlags::normal).raw;
        lut[0][i + 8] = Move::make(PieceId{0}, Square{narrow_cast<u8>(wsrc + 16)}, MoveFlags::double_push).raw;
        const u8 bsrc = 48 + i;
        lut[1][i + 0] = Move::make(PieceId{0}, Square{narrow_cast<u8>(bsrc - 8)}, MoveFlags::normal).raw;
        lut[1][i + 8] = Move::make(PieceId{0}, Square{narrow_cast<u8>(bsrc - 16)}, MoveFlags::double_push).raw;
      }
      return std::array<v256, 2>{v256{lut[0]}, v256{lut[1]}};
    }();

    switch (perspective.raw) {
    case Color::white:
      return vec::permute8_mz(0x55555555, v512::from256(v256{std::array<u8, 32>{8, 0, 9, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 0,
                                                                                8, 0, 9, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 0}}),
                              ids)
                 .to256() |
             lut[0];
    case Color::black:
      return vec::permute8_mz(0x55555555, v512::from256(v256{std::array<u8, 32>{48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0,
                                                                                48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0}}),
                              ids)
                 .to256() |
             lut[1];
    }
    std::unreachable();
  }

} // namespace rose::pawns
