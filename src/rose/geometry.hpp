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
    inline auto compress_coords(u8x64 list) -> std::tuple<u8x64, m8x64> {
      const m8x64 valid = (list & u8x64::splat(0x88)).zeros();
#if LPS_AVX512
      const u8x64 compressed {_mm512_gf2p8affine_epi64_epi8(list.raw, u64x8::splat(0x0102041020400000).raw, 0)};
#else
      const u8x64 compressed = (list & u8x64::splat(0x07)) | (list & u8x64::splat(0x70)).shr<1>();
#endif
      return {compressed, valid};
    }

  }  // namespace internal

  inline auto superpiece_rays(Square sq) -> std::tuple<u8x64, m8x64> {
    constexpr u8x64 offsets {{
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

  template<u8x64 offsets>
  inline auto superpiece_inverse_rays_base(Square sq) -> u8x64 {
    constexpr u8 none = 0xFF;
    constexpr u8x64 table0 {{
      none, none, none, none, none, none, none, none, none, none, none, none, none, none, none, none,  //
      none, 0x2F, none, none, none, none, none, none, 0x27, none, none, none, none, none, none, 0x1F,  //
      none, none, 0x2E, none, none, none, none, none, 0x26, none, none, none, none, none, 0x1E, none,  //
      none, none, none, 0x2D, none, none, none, none, 0x25, none, none, none, none, 0x1D, none, none,  //
    }};
    constexpr u8x64 table1 {{
      none, none, none, none, 0x2C, none, none, none, 0x24, none, none, none, 0x1C, none, none, none,  //
      none, none, none, none, none, 0x2B, none, none, 0x23, none, none, 0x1B, none, none, none, none,  //
      none, none, none, none, none, none, 0x2A, 0x28, 0x22, 0x20, 0x1A, none, none, none, none, none,  //
      none, none, none, none, none, none, 0x30, 0x29, 0x21, 0x19, 0x18, none, none, none, none, none,  //
    }};
    constexpr u8x64 table2 {{
      none, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, none, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  //
      none, none, none, none, none, none, 0x38, 0x39, 0x01, 0x09, 0x10, none, none, none, none, none,  //
      none, none, none, none, none, none, 0x3A, 0x00, 0x02, 0x08, 0x0A, none, none, none, none, none,  //
      none, none, none, none, none, 0x3B, none, none, 0x03, none, none, 0x0B, none, none, none, none,  //
    }};
    constexpr u8x64 table3 {{
      none, none, none, none, 0x3C, none, none, none, 0x04, none, none, none, 0x0C, none, none, none,  //
      none, none, none, 0x3D, none, none, none, none, 0x05, none, none, none, none, 0x0D, none, none,  //
      none, none, 0x3E, none, none, none, none, none, 0x06, none, none, none, none, none, 0x0E, none,  //
      none, 0x3F, none, none, none, none, none, none, 0x07, none, none, none, none, none, none, 0x0F,  //
    }};

    const u8x64 indexes = offsets - u8x64::splat(internal::expand_sq(sq));
    const u8x64 result0 = (indexes & u8x64::splat(0x7F)).swizzle(table0, table1);
    const u8x64 result1 = (indexes & u8x64::splat(0x7F)).swizzle(table2, table3);
    return indexes.msb().select(result0, result1);
  }

  inline auto superpiece_inverse_rays(Square sq) -> u8x64 {
    return superpiece_inverse_rays_base<u8x64 {{
      0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,  // north
      0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,  // north-east
      0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,  // east
      0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,  // south-east
      0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,  // south
      0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,  // south-west
      0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,  // west
      0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,  // north-west
    }}>(sq);
  }

  template<u8x64 offsets>
  inline auto superpiece_inverse_rays_flipped_base(Square sq) -> u8x64 {
    constexpr u8 none = 0xFF;
    constexpr u8x64 table0 {{
      none, none, none, none, none, none, none, none, none, none, none, none, none, none, none, none,  //
      none, 0x0F, none, none, none, none, none, none, 0x07, none, none, none, none, none, none, 0x3F,  //
      none, none, 0x0E, none, none, none, none, none, 0x06, none, none, none, none, none, 0x3E, none,  //
      none, none, none, 0x0D, none, none, none, none, 0x05, none, none, none, none, 0x3D, none, none,  //
    }};
    constexpr u8x64 table1 {{
      none, none, none, none, 0x0C, none, none, none, 0x04, none, none, none, 0x3C, none, none, none,  //
      none, none, none, none, none, 0x0B, none, none, 0x03, none, none, 0x3B, none, none, none, none,  //
      none, none, none, none, none, none, 0x0A, 0x08, 0x02, 0x00, 0x3A, none, none, none, none, none,  //
      none, none, none, none, none, none, 0x10, 0x09, 0x01, 0x39, 0x38, none, none, none, none, none,  //
    }};
    constexpr u8x64 table2 {{
      none, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, none, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  //
      none, none, none, none, none, none, 0x18, 0x19, 0x21, 0x29, 0x30, none, none, none, none, none,  //
      none, none, none, none, none, none, 0x1A, 0x20, 0x22, 0x28, 0x2A, none, none, none, none, none,  //
      none, none, none, none, none, 0x1B, none, none, 0x23, none, none, 0x2B, none, none, none, none,  //
    }};
    constexpr u8x64 table3 {{
      none, none, none, none, 0x1C, none, none, none, 0x24, none, none, none, 0x2C, none, none, none,  //
      none, none, none, 0x1D, none, none, none, none, 0x25, none, none, none, none, 0x2D, none, none,  //
      none, none, 0x1E, none, none, none, none, none, 0x26, none, none, none, none, none, 0x2E, none,  //
      none, 0x1F, none, none, none, none, none, none, 0x27, none, none, none, none, none, none, 0x2F,  //
    }};

    const u8x64 indexes = offsets - u8x64::splat(internal::expand_sq(sq));
    const u8x64 result0 = (indexes & u8x64::splat(0x7F)).swizzle(table0, table1);
    const u8x64 result1 = (indexes & u8x64::splat(0x7F)).swizzle(table2, table3);
    return indexes.msb().select(result0, result1);
  }

  inline auto superpiece_inverse_rays_flipped(Square sq) -> u8x64 {
    return superpiece_inverse_rays_flipped_base<u8x64 {{
      0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,  // north
      0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,  // north-east
      0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,  // east
      0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,  // south-east
      0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,  // south
      0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,  // south-west
      0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,  // west
      0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,  // north-west
    }}>(sq);
  }

  auto superpiece_attacks(u8x64 ray_places, m8x64 ray_valid) -> m8x64;
  auto superpiece_attacks(m8x64 ray_places, m8x64 ray_valid) -> m8x64;

  inline auto superpiece_attacks(u8x64 ray_places, m8x64 ray_valid) -> m8x64 {
#if LPS_AVX512
    return superpiece_attacks(ray_places.nonzeros(), ray_valid);
#else
    return ray_valid.andnot(ray_places.eq(std::bit_cast<u8x64>(std::bit_cast<u64x8>(ray_places) - u64x8::splat(0x101))));
#endif
  }

  inline auto superpiece_attacks(m8x64 occupied, m8x64 ray_valid) -> m8x64 {
#if LPS_AVX512
    const u64 o = occupied.raw | 0x8181818181818181;
    const u64 x = o ^ (o - 0x0303030303030303);
    return m8x64 {x} & ray_valid;
#else
    return superpiece_attacks(occupied.to_vector().convert<u8>(), ray_valid);
#endif
  }

  inline auto attackers_from_rays(u8x64 ray_places) -> m8x64 {
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

    constexpr u8x16 ptype_to_bits {{0, k, wp, n, 0, b, r, q, 0, k, bp, n, 0, b, r, q}};
    constexpr u8x64 base {{
      horse, oadj, orth, orth, orth, orth, orth, orth,  // north
      horse, bpdj, diag, diag, diag, diag, diag, diag,  // north-east
      horse, oadj, orth, orth, orth, orth, orth, orth,  // east
      horse, wpdj, diag, diag, diag, diag, diag, diag,  // south-east
      horse, oadj, orth, orth, orth, orth, orth, orth,  // south
      horse, wpdj, diag, diag, diag, diag, diag, diag,  // south-west
      horse, oadj, orth, orth, orth, orth, orth, orth,  // west
      horse, bpdj, diag, diag, diag, diag, diag, diag,  // north-west
    }};

    const u8x64 bit_rays = ray_places.shr<4>().swizzle(ptype_to_bits);
    return bit_rays.test(base);
  }

  inline auto attack_mask(Color color, PieceType ptype) -> m8x64 {
    constexpr auto gen_base = [](Color color) consteval {
      const u8 k = 1 << PieceType::k;
      const u8 wp = color == Color::white ? 1 << PieceType::p : 0;
      const u8 bp = color == Color::black ? 1 << PieceType::p : 0;
      const u8 n = 1 << PieceType::n;
      const u8 b = 1 << PieceType::b;
      const u8 r = 1 << PieceType::r;
      const u8 q = 1 << PieceType::q;

      const u8 diag = b | q;
      const u8 orth = r | q;
      const u8 oadj = r | q | k;
      const u8 horse = n;
      const u8 wpdj = b | q | k | wp;
      const u8 bpdj = b | q | k | bp;

      return u8x64 {{
        horse, oadj, orth, orth, orth, orth, orth, orth,  // north
        horse, wpdj, diag, diag, diag, diag, diag, diag,  // north-east
        horse, oadj, orth, orth, orth, orth, orth, orth,  // east
        horse, bpdj, diag, diag, diag, diag, diag, diag,  // south-east
        horse, oadj, orth, orth, orth, orth, orth, orth,  // south
        horse, bpdj, diag, diag, diag, diag, diag, diag,  // south-west
        horse, oadj, orth, orth, orth, orth, orth, orth,  // west
        horse, wpdj, diag, diag, diag, diag, diag, diag,  // north-west
      }};
    };

    constexpr std::array<u8x64, 2> bases {{
      gen_base(Color::white),
      gen_base(Color::black),
    }};

    const u8 bit = narrow_cast<u8>(1 << ptype.raw);
    return bases[color.to_index()].test(u8x64::splat(bit));
  }

  inline auto sliders_from_rays(u8x64 rays) -> m8x64 {
    constexpr u8 slider_bit = 0b100 << 4;
    constexpr u8 diag = 0b001 << 4;
    constexpr u8 orth = 0b010 << 4;
    constexpr u8x64 slider_mask {{
      0, orth, orth, orth, orth, orth, orth, orth,  // north
      0, diag, diag, diag, diag, diag, diag, diag,  // north-east
      0, orth, orth, orth, orth, orth, orth, orth,  // east
      0, diag, diag, diag, diag, diag, diag, diag,  // south-east
      0, orth, orth, orth, orth, orth, orth, orth,  // south
      0, diag, diag, diag, diag, diag, diag, diag,  // south-west
      0, orth, orth, orth, orth, orth, orth, orth,  // west
      0, diag, diag, diag, diag, diag, diag, diag,  // north-west
    }};
    return rays.test(u8x64::splat(slider_bit)) & rays.test(slider_mask);
  }

  inline auto slider_broadcast(u8x64 x) -> u8x64 {
#if LPS_AVX512
    return {_mm512_maskz_gf2p8affine_epi64_epi8(
      0xFEFEFEFEFEFEFEFE, _mm512_set1_epi8(-1), _mm512_gf2p8affine_epi64_epi8(_mm512_set1_epi64(0x0102040810204080), x.raw, 0), 0)};
#elif LPS_AVX2
    u8x32 expand {{
      0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x80, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,  //
      0x80, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  //
      0x80, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,  //
    }};
    u8x64 y;
    y.raw[0].raw = _mm256_sad_epu8(x.raw[0].raw, _mm256_setzero_si256());
    y.raw[1].raw = _mm256_sad_epu8(x.raw[1].raw, _mm256_setzero_si256());
    y.raw[0] = expand.swizzle(y.raw[0]);
    y.raw[1] = expand.swizzle(y.raw[1]);
    return y;
#else
    u64x8 y = std::bit_cast<u64x8>(x);
    y *= u64x8::splat(0x0101010101010101);
    y = y.shr<56>();
    y *= u64x8::splat(0x0101010101010100);
    return std::bit_cast<u8x64>(y);
#endif
  }

  inline auto lane_broadcast(u8x64 x) -> u8x64 {
#if LPS_AVX512
    return {_mm512_gf2p8affine_epi64_epi8(_mm512_set1_epi8(-1), _mm512_gf2p8affine_epi64_epi8(_mm512_set1_epi64(0x0102040810204080), x.raw, 0), 0)};
#elif LPS_AVX2
    u8x32 expand {{
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,  //
      0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  //
      0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,  //
    }};
    u8x64 y;
    y.raw[0].raw = _mm256_sad_epu8(x.raw[0].raw, _mm256_setzero_si256());
    y.raw[1].raw = _mm256_sad_epu8(x.raw[1].raw, _mm256_setzero_si256());
    y.raw[0] = expand.swizzle(y.raw[0]);
    y.raw[1] = expand.swizzle(y.raw[1]);
    return y;
#else
    u64x8 y = std::bit_cast<u64x8>(x);
    y *= u64x8::splat(0x0101010101010101);
    y = y.shr<56>();
    y *= u64x8::splat(0x0101010101010101);
    return std::bit_cast<u8x64>(y);
#endif
  }

  inline auto flip_rays(u8x64 x) -> u8x64 {
#if LPS_AVX512
    return u8x64 {_mm512_shuffle_i64x2(x.raw, x.raw, 0b01001110)};
#else
    auto y = std::bit_cast<std::array<u8x32, 2> >(x);
    return std::bit_cast<u8x64>(std::array<u8x32, 2> {y[1], y[0]});
#endif
  }

  inline auto flip_mask(m8x64 x) -> m8x64 {
#if LPS_AVX512
    return m8x64 {std::rotl(x.raw, 32)};
#else
    auto y = std::bit_cast<std::array<m8x32, 2> >(x);
    return std::bit_cast<m8x64>(std::array<m8x32, 2> {y[1], y[0]});
#endif
  }

  inline auto find_set(u8x16 needle, usize needle_count, u8x16 haystack) -> PieceMask {
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
