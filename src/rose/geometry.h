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
        0xDF, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, // north-east
        0xE1, 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, // north-west
        0xEE, 0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97, // south-east
        0x0E, 0xEF, 0xDE, 0xCD, 0xBC, 0xAB, 0x9A, 0x89, // south-west
        0xF2, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // east
        0x12, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, // north
        0x1F, 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, // west
        0x21, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, // south
        // clang-format on
    });
    const v512 uncompressed = vec::add8(v512::broadcast8(internal::expandSq(sq)), offsets);
    return internal::compressCoords(uncompressed);
  }

  inline auto superpieceAttacks(u64 occupied, u64 rayvalid) -> u64 {
    const u64 o = occupied | 0x8181818181818181;
    const u64 x = o ^ (o - 0x0303030303030303);
    return x & rayvalid;
  }

  inline constexpr u64 non_horse_attack_mask = 0xFEFEFEFEFEFEFEFE;

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
          horse, pawn, diag, diag, diag, diag, diag, diag,
          horse, pawn, diag, diag, diag, diag, diag, diag,
          horse, dadj, diag, diag, diag, diag, diag, diag,
          horse, dadj, diag, diag, diag, diag, diag, diag,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          // clang-format on
      });
    case Color::black:
      return v512::fromArray({
          // clang-format off
          horse, dadj, diag, diag, diag, diag, diag, diag,
          horse, dadj, diag, diag, diag, diag, diag, diag,
          horse, pawn, diag, diag, diag, diag, diag, diag,
          horse, pawn, diag, diag, diag, diag, diag, diag,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          horse, oadj, orth, orth, orth, orth, orth, orth,
          // clang-format on
      });
    }

    std::unreachable();
  }

  inline auto superpieceInverseRays(Square sq) -> v512 {
    // clang-format off
    constexpr u8 none = 0xFF;
    const v512 table0 = v512::fromArray({
        none, none, none, none, none, none, none, none, none, none, none, none, none, none, none, none,
        none, 0x1F, none, none, none, none, none, none, 0x3F, none, none, none, none, none, none, 0x17,
        none, none, 0x1E, none, none, none, none, none, 0x3E, none, none, none, none, none, 0x16, none,
        none, none, none, 0x1D, none, none, none, none, 0x3D, none, none, none, none, 0x15, none, none,
    });
    const v512 table1 = v512::fromArray({
        none, none, none, none, 0x1C, none, none, none, 0x3C, none, none, none, 0x14, none, none, none,
        none, none, none, none, none, 0x1B, none, none, 0x3B, none, none, 0x13, none, none, none, none,
        none, none, none, none, none, none, 0x1A, 0x00, 0x3A, 0x08, 0x12, none, none, none, none, none,
        none, none, none, none, none, none, 0x10, 0x19, 0x39, 0x11, 0x20, none, none, none, none, none,
    });
    const v512 table2 = v512::fromArray({
        none, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, none, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        none, none, none, none, none, none, 0x18, 0x09, 0x29, 0x01, 0x28, none, none, none, none, none,
        none, none, none, none, none, none, 0x0A, 0x30, 0x2A, 0x38, 0x02, none, none, none, none, none,
        none, none, none, none, none, 0x0B, none, none, 0x2B, none, none, 0x03, none, none, none, none,
    });
    const v512 table3 = v512::fromArray({
        none, none, none, none, 0x0C, none, none, none, 0x2C, none, none, none, 0x04, none, none, none,
        none, none, none, 0x0D, none, none, none, none, 0x2D, none, none, none, none, 0x05, none, none,
        none, none, 0x0E, none, none, none, none, none, 0x2E, none, none, none, none, none, 0x06, none,
        none, 0x0F, none, none, none, none, none, none, 0x2F, none, none, none, none, none, none, 0x07,
    });
    const v512 offsets = v512::fromArray({
        0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
        0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
        0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
        0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
        0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
        0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
        0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
        0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,
    });
    // clang-format on

    const v512 indexes = vec::sub8(offsets, v512::broadcast8(internal::expandSq(sq)));
    const v512 result0 = vec::permute8(indexes, table0, table1);
    const v512 result1 = vec::permute8(indexes, table2, table3);
    return vec::blend8(indexes.msb8(), result0, result1);
  }

} // namespace rose::geometry
