#pragma once

#include <utility>

#include "rose/square.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose::geometry {

  namespace internal {
    // 00rrrfff → 0rrr0fff
    inline auto expandSq(Square sq) -> u8 { return sq.raw + (sq.raw & 0b111000); }

    // 0rrr0fff → 00rrrfff
    template <typename V> inline auto compressCoords(V list) -> std::tuple<V, typename V::Mask8> {
      const typename V::Mask8 valid = vec::testn8(list, V::broadcast8(0x88));
      const V compressed = vec::gf2p8affine8(list, V::broadcast64(0x0102041020400000), 0);
      return {compressed, valid};
    }
  } // namespace internal

  inline auto superpieceRays(Square sq) -> std::tuple<v512, u64> {
    const v512 offsets = v512::fromArray({
        // clang-format off
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69,
        0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,
        0xEF, 0xDE, 0xCD, 0xBC, 0xAB, 0x9A, 0x89,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
        0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9,
        0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,
        0xDF, 0xE1, 0xEE, 0x0E, 0xF2, 0x12, 0x1F, 0x21,
        // clang-format on
    });
    const v512 uncompressed = vec::add8(v512::broadcast8(internal::expandSq(sq)), offsets);
    return internal::compressCoords(uncompressed);
  }

  inline auto superpieceAttacks(u64 occupied, u64 rayvalid) -> u64 {
    const u64 o = occupied | 0xFF81020408102040;
    const u64 x = o ^ (o - 0xFF02040810204081);
    return x & rayvalid;
  }

  inline auto superpieceAttackerMask(Color color) -> v512 {
    constexpr u8 diag = PieceType::b;
    constexpr u8 orth = PieceType::r;
    constexpr u8 pawn = PieceType::b | PieceType::p | PieceType::k;
    constexpr u8 dadj = PieceType::b | PieceType::k;
    constexpr u8 oadj = PieceType::r | PieceType::k;
    constexpr u8 horse = PieceType::n;

    switch (color.raw) {
    case Color::white:
      return v512::fromArray({
          // clang-format off
          pawn, diag, diag, diag, diag, diag, diag,
          pawn, diag, diag, diag, diag, diag, diag,
          dadj, diag, diag, diag, diag, diag, diag,
          dadj, diag, diag, diag, diag, diag, diag,
          oadj, orth, orth, orth, orth, orth, orth,
          oadj, orth, orth, orth, orth, orth, orth,
          oadj, orth, orth, orth, orth, orth, orth,
          oadj, orth, orth, orth, orth, orth, orth,
          horse, horse, horse, horse, horse, horse, horse, horse,
          // clang-format on
      });
    case Color::black:
      return v512::fromArray({
          // clang-format off
          dadj, diag, diag, diag, diag, diag, diag,
          dadj, diag, diag, diag, diag, diag, diag,
          pawn, diag, diag, diag, diag, diag, diag,
          pawn, diag, diag, diag, diag, diag, diag,
          oadj, orth, orth, orth, orth, orth, orth,
          oadj, orth, orth, orth, orth, orth, orth,
          oadj, orth, orth, orth, orth, orth, orth,
          oadj, orth, orth, orth, orth, orth, orth,
          horse, horse, horse, horse, horse, horse, horse, horse,
          // clang-format on
      });
    }

    std::unreachable();
  }

  //  inline auto queenRays() -> v512 {
  //    const v512 offsets = v512::fromArray({
  //        // clang-format off
  //          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  //          0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79,
  //          0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,
  //          0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
  //          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  //          0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
  //          0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
  //          0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,
  //          0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
  //        // clang-format on
  //    });
  //    return vec::add8(v512::broadcast8(raw), offsets);
  //  }
  //
  //  inline auto orthogonalRays() -> v256 {
  //    const v256 offsets = v256::fromArray({
  //        // clang-format off
  //          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  //          0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
  //          0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
  //          0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,
  //          0x88, 0x88, 0x88, 0x88,
  //        // clang-format on
  //    });
  //    return vec::add8(v256::broadcast8(raw), offsets);
  //  }
  //
  //  inline auto diagonalRays() -> v256 {
  //    const v256 offsets = v256::fromArray({
  //        // clang-format off
  //          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  //          0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79,
  //          0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,
  //          0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
  //          0x88, 0x88, 0x88, 0x88,
  //        // clang-format on
  //    });
  //    return vec::add8(v256::broadcast8(raw), offsets);
  //  }
  //
  //  inline auto kingLeaps() -> v128 {
  //    const v128 offsets = v128::fromArray({
  //        // clang-format off
  //          0x01, 0x10, 0x0F, 0xF0, 0x11, 0x1F, 0xF1, 0xFF,
  //          0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
  //        // clang-format on
  //    });
  //    return vec::add8(v128::broadcast8(raw), offsets);
  //  }
  //
  //  inline auto knightLeaps() -> v128 {
  //    const v128 offsets = v128::fromArray({
  //        // clang-format off
  //          0xDF, 0xE1, 0xEE, 0x0E, 0xF2, 0x12, 0x1F, 0x21,
  //          0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
  //        // clang-format on
  //    });
  //    return vec::add8(v128::broadcast8(raw), offsets);
  //  }

} // namespace rose::geometry
