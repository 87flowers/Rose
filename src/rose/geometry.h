#pragma once

#include <utility>

#include "rose/square.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose::geometry {

  namespace internal {
    // 00rrrfff → 0rrr0fff
    forceinline constexpr auto expandSq(Square sq) -> u8 { return sq.raw + (sq.raw & 0b111000); }

    // 0rrr0fff → 00rrrfff
    template <typename V> forceinline auto compressCoords(V list) -> std::tuple<V, typename V::Mask8> {
      const typename V::Mask8 valid = vec::testn8(list, V::broadcast8(0x88));
      const V compressed = vec::gf2p8matmul8(list, V::broadcast64(0x0102041020400000));
      return {compressed, valid};
    }

    template <typename V> forceinline auto compressCoordsWide(V list) -> std::tuple<V, typename V::Mask16> {
      const typename V::Mask8 valid = vec::testn16(list, V::broadcast16(0xFF88));
      const V compressed = vec::gf2p8matmul8(list, V::broadcast64(0x0102041020400000));
      return {compressed, valid};
    }
  } // namespace internal

  forceinline auto superpieceRays(Square sq) -> std::tuple<v512, u64> {
    constexpr v512 offsets = v512{std::array<u8, 64>{
        // clang-format off
        0x1F, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, // north
        0x21, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, // north-east
        0x12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // east
        0xF2, 0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97, // south-east
        0xE1, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, // south
        0xDF, 0xEF, 0xDE, 0xCD, 0xBC, 0xAB, 0x9A, 0x89, // south-west
        0xEE, 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, // west
        0x0E, 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, // north-west
        // clang-format on
    }};
    const v512 uncompressed = vec::add8(v512::broadcast8(internal::expandSq(sq)), offsets);
    return internal::compressCoords(uncompressed);
  }

  inline constexpr u16 adjacents_same_file_mask = 0b00010001;
  inline constexpr u16 adjacents_same_rank_mask = 0b01000100;
  inline constexpr u16 adjacents_diagonal_mask = 0b00100010;
  inline constexpr u16 adjacents_antidiagonal_mask = 0b10001000;

  forceinline auto adjacents(Square sq) -> std::tuple<v128, u16> {
    //                                                  n,   ne,    e,   se,    s,   sw,    w,   nw,
    constexpr v128 offsets = v128{std::array<u8, 16>{0x10, 0x11, 0x01, 0xF1, 0xF0, 0xEF, 0xFF, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88}};
    const v128 uncompressed = vec::add8(v128::broadcast8(internal::expandSq(sq)), offsets);
    return internal::compressCoords(uncompressed);
  }

  forceinline auto adjacentsWide(Square sq) -> std::tuple<v128, u8> {
    constexpr v128 offsets = v128{std::array<u16, 8>{0x0F, 0xF1, 0x11, 0xEF, 0x10, 0xF0, 0x01, 0xFF}};
    const v128 uncompressed = vec::add16(v128::broadcast16(internal::expandSq(sq)), offsets);
    return internal::compressCoordsWide(uncompressed);
  }

  forceinline auto superpieceAttacks(u64 occupied, u64 rayvalid) -> u64 {
    const u64 o = occupied | 0x8181818181818181;
    const u64 x = o ^ (o - 0x0303030303030303);
    return x & rayvalid;
  }

  inline constexpr u64 non_horse_attack_mask = 0xFEFEFEFEFEFEFEFE;

  inline constexpr v512 sliderMask = [] {
    constexpr u8 diag = 0b001 << 4;
    constexpr u8 orth = 0b010 << 4;
    return v512{std::array<u8, 64>{
        // clang-format off
        0, orth, orth, orth, orth, orth, orth, orth,
        0, diag, diag, diag, diag, diag, diag, diag,
        0, orth, orth, orth, orth, orth, orth, orth,
        0, diag, diag, diag, diag, diag, diag, diag,
        0, orth, orth, orth, orth, orth, orth, orth,
        0, diag, diag, diag, diag, diag, diag, diag,
        0, orth, orth, orth, orth, orth, orth, orth,
        0, diag, diag, diag, diag, diag, diag, diag,
        // clang-format on
    }};
  }();

  forceinline auto slidersFromRays(v512 rays) -> u64 {
    return (rays & v512::broadcast8(0b100 << 4)).nonzero8() & (rays & geometry::sliderMask).nonzero8();
  }

  inline constexpr std::array<u64, 16> attackMaskTable = [] {
    constexpr int k = (1 << PieceType::k) | (0x100 << PieceType::k);
    constexpr int wp = (1 << PieceType::p);
    constexpr int bp = (0x100 << PieceType::p);
    constexpr int n = (1 << PieceType::n) | (0x100 << PieceType::n);
    constexpr int b = (1 << PieceType::b) | (0x100 << PieceType::b);
    constexpr int r = (1 << PieceType::r) | (0x100 << PieceType::r);
    constexpr int q = (1 << PieceType::q) | (0x100 << PieceType::q);

    constexpr int diag = b | q;
    constexpr int orth = r | q;
    constexpr int oadj = r | q | k;
    constexpr int horse = n;
    constexpr int wpdj = b | q | k | wp;
    constexpr int bpdj = b | q | k | bp;

    constexpr std::array<int, 64> base{{
        // clang-format off
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, wpdj, diag, diag, diag, diag, diag, diag,
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, bpdj, diag, diag, diag, diag, diag, diag,
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, bpdj, diag, diag, diag, diag, diag, diag,
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, wpdj, diag, diag, diag, diag, diag, diag,
        // clang-format on
    }};

    std::array<u64, 16> table{};
    for (int pt = 0; pt < 16; pt++) {
      const int pt_mask = 1 << pt;
      for (int sq = 0; sq < 64; sq++) {
        const u64 bit = static_cast<u64>(1) << sq;
        if (base[sq] & pt_mask)
          table[pt] |= bit;
      }
    }
    return table;
  }();

  forceinline auto attackersFromRays(v512 rays) -> u64 {
    constexpr u8 k = 1 << 0;
    constexpr u8 wp = 1 << 1;
    constexpr u8 bp = 1 << 2;
    constexpr u8 n = 1 << 3;
    constexpr u8 b = 1 << 4;
    constexpr u8 r = 1 << 5;
    constexpr u8 q = 1 << 6;

    constexpr u8 diag = b | q;
    constexpr u8 orth = r | q;
    constexpr u8 oadj = r | q | k;
    constexpr u8 horse = n;
    constexpr u8 wpdj = b | q | k | wp;
    constexpr u8 bpdj = b | q | k | bp;

    static constexpr v128 ptype_to_bits{std::array<u8, 16>{{0, k, bp, n, 0, b, r, q, 0, k, wp, n, 0, b, r, q}}};
    static constexpr v512 base{std::array<u8, 64>{{
        // clang-format off
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, wpdj, diag, diag, diag, diag, diag, diag,
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, bpdj, diag, diag, diag, diag, diag, diag,
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, bpdj, diag, diag, diag, diag, diag, diag,
        horse, oadj, orth, orth, orth, orth, orth, orth,
        horse, wpdj, diag, diag, diag, diag, diag, diag,
        // clang-format on
    }}};

    // TODO: PSHUFB instead
    const v512 bit_rays = vec::permute8(vec::shr16(rays, 4) & v512::broadcast8(0x0F), v512::from128(ptype_to_bits));
    return (bit_rays & base).nonzero8();
  }

  inline constexpr std::array<v512, 64> superpieceInverseRaysTable = [] {
    // clang-format off
    constexpr u8 none = 0xFF;
    constexpr std::array<u8, 256> base{{
        none, none, none, none, none, none, none, none, none, none, none, none, none, none, none, none,
        none, 0x2F, none, none, none, none, none, none, 0x27, none, none, none, none, none, none, 0x1F,
        none, none, 0x2E, none, none, none, none, none, 0x26, none, none, none, none, none, 0x1E, none,
        none, none, none, 0x2D, none, none, none, none, 0x25, none, none, none, none, 0x1D, none, none,
        none, none, none, none, 0x2C, none, none, none, 0x24, none, none, none, 0x1C, none, none, none,
        none, none, none, none, none, 0x2B, none, none, 0x23, none, none, 0x1B, none, none, none, none,
        none, none, none, none, none, none, 0x2A, 0x28, 0x22, 0x20, 0x1A, none, none, none, none, none,
        none, none, none, none, none, none, 0x30, 0x29, 0x21, 0x19, 0x18, none, none, none, none, none,
        none, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, none, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        none, none, none, none, none, none, 0x38, 0x39, 0x01, 0x09, 0x10, none, none, none, none, none,
        none, none, none, none, none, none, 0x3A, 0x00, 0x02, 0x08, 0x0A, none, none, none, none, none,
        none, none, none, none, none, 0x3B, none, none, 0x03, none, none, 0x0B, none, none, none, none,
        none, none, none, none, 0x3C, none, none, none, 0x04, none, none, none, 0x0C, none, none, none,
        none, none, none, 0x3D, none, none, none, none, 0x05, none, none, none, none, 0x0D, none, none,
        none, none, 0x3E, none, none, none, none, none, 0x06, none, none, none, none, none, 0x0E, none,
        none, 0x3F, none, none, none, none, none, none, 0x07, none, none, none, none, none, none, 0x0F,
    }};
    constexpr std::array<u8, 64> offsets{{
        0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
        0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
        0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
        0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
        0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
        0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
        0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
        0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,
    }};
    // clang-format on

    std::array<v512, 64> table;
    for (u8 sq = 0; sq < 64; sq++) {
      const u8 esq = internal::expandSq(Square{sq});
      std::array<u8, 64> b;
      for (int i = 0; i < 64; i++) {
        b[i] = base[offsets[i] - esq];
      }
      table[sq] = v512{b};
    }
    return table;
  }();

  forceinline auto superpieceInverseRays(Square sq) -> v512 { return superpieceInverseRaysTable[sq.raw]; }

  inline constexpr std::array<v512, 64> superpieceInverseRaysSwappedTable = [] {
    // clang-format off
    constexpr u8 none = 0xFF;
    constexpr std::array<u8, 256> base{{
        none, none, none, none, none, none, none, none, none, none, none, none, none, none, none, none,
        none, 0x2F, none, none, none, none, none, none, 0x27, none, none, none, none, none, none, 0x1F,
        none, none, 0x2E, none, none, none, none, none, 0x26, none, none, none, none, none, 0x1E, none,
        none, none, none, 0x2D, none, none, none, none, 0x25, none, none, none, none, 0x1D, none, none,
        none, none, none, none, 0x2C, none, none, none, 0x24, none, none, none, 0x1C, none, none, none,
        none, none, none, none, none, 0x2B, none, none, 0x23, none, none, 0x1B, none, none, none, none,
        none, none, none, none, none, none, 0x2A, 0x28, 0x22, 0x20, 0x1A, none, none, none, none, none,
        none, none, none, none, none, none, 0x30, 0x29, 0x21, 0x19, 0x18, none, none, none, none, none,
        none, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, none, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        none, none, none, none, none, none, 0x38, 0x39, 0x01, 0x09, 0x10, none, none, none, none, none,
        none, none, none, none, none, none, 0x3A, 0x00, 0x02, 0x08, 0x0A, none, none, none, none, none,
        none, none, none, none, none, 0x3B, none, none, 0x03, none, none, 0x0B, none, none, none, none,
        none, none, none, none, 0x3C, none, none, none, 0x04, none, none, none, 0x0C, none, none, none,
        none, none, none, 0x3D, none, none, none, none, 0x05, none, none, none, none, 0x0D, none, none,
        none, none, 0x3E, none, none, none, none, none, 0x06, none, none, none, none, none, 0x0E, none,
        none, 0x3F, none, none, none, none, none, none, 0x07, none, none, none, none, none, none, 0x0F,
    }};
    constexpr std::array<u8, 64> offsets{{
        0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
        0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
        0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
        0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
        0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
        0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
        0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
        0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,
    }};
    // clang-format on

    std::array<v512, 64> table;
    for (u8 sq = 0; sq < 64; sq++) {
      const u8 esq = internal::expandSq(Square{sq});
      std::array<u8, 64> b;
      for (int i = 0; i < 64; i++) {
        const u8 j = base[offsets[i] - esq];
        b[i] = j != none ? (j + 32) % 64 : none;
      }
      table[sq] = v512{b};
    }
    return table;
  }();

  forceinline auto superpieceInverseRaysSwapped(Square sq) -> v512 { return superpieceInverseRaysSwappedTable[sq.raw]; }

} // namespace rose::geometry
