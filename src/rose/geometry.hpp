#pragma once

#include "rose/board.hpp"
#include "rose/common.hpp"

namespace rose::geometry {

  namespace internal {
    // 00rrrfff → 0rrr0fff
    constexpr auto expand_sq(Square sq) -> u8 {
      return sq.raw + (sq.raw & 0b111000);
    }

    // 0rrr0fff → 00rrrfff
    auto compress_coords(u8x64 list) -> std::tuple<u8x64, m8x64> {
      const m8x64 valid = (list & u8x64::splat(0x88)).zeros();
#if LPS_AVX512
      const u8x64 compressed {_mm512_gf2p8affine_epi64_epi8(list.raw, u64x8::splat(0x0102041020400000).raw, 0)};
#else
      const u8x64 compressed = (list & u8x64::splat(0x07)) | (list & u8x64::splat(0x70)).shr<1>();
#endif
      return {compressed, valid};
    }

  }  // namespace internal

  auto superpiece_rays(Square sq) -> std::tuple<u8x64, m8x64> {
    static const u8x64 offsets {{
      0x1F, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,  // north
      0x21, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,  // north-east
      0x12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // east
      0xF2, 0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,  // south-east
      0xE1, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,  // south
      0xDF, 0xEF, 0xDE, 0xCD, 0xBC, 0xAB, 0x9A, 0x89,  // south-west
      0xEE, 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9,  // west
      0x0E, 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69,  // north-west
    }};
    const u8x64 uncompressed = u8x64::splat(internal::expand_sq(sq)) + offsets;
    return internal::compress_coords(uncompressed);
  }

  auto superpiece_attacks(u8x64 ray_places, m8x64 ray_valid) -> m8x64 {
#if LPS_AVX512
    const u64 occupied = ray_places.nonzeros().raw;
    const u64 o = occupied | 0x8181818181818181;
    const u64 x = o ^ (o - 0x0303030303030303);
    return m8x64 {x} & ray_valid;
#else
    return ray_valid.andnot(ray_places.eq(std::bit_cast<u8x64>(std::bit_cast<u64x8>(ray_places) - u64x8::splat(0x101))));
#endif
  }

  auto attackers_from_rays(u8x64 ray_places) -> m8x64 {
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

    static constexpr u8x16 ptype_to_bits {{0, k, bp, n, 0, b, r, q, 0, k, wp, n, 0, b, r, q}};
    static constexpr u8x64 base {{
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

    const u8x64 bit_rays = ray_places.shr<4>().swizzle(ptype_to_bits);
    return bit_rays.test(base);
  }

  auto find_set(u8x16 needle, usize needle_count, u8x16 haystack) -> PieceMask {
#if LPS_SSE4_2 || LPS_AVX2 || LPS_AVX512
    return PieceMask {narrow_cast<u16>(_mm_extract_epi16(_mm_cmpestrm(needle.raw, needle_count, haystack.raw, 16, 0), 0))};
#else
    u16 result = 0;
    for (usize i = 0; i < needle_count; ++i) {
      result |= haystack.eq(u8x16::splat(needle.read(i))).to_bits();
    }
    return PieceMask {result};
#endif
  }

}  // namespace rose::geometry
