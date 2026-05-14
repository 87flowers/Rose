#pragma once

#include "rose/common.hpp"
#include "rose/geometry.hpp"
#include "rose/move.hpp"
#include "rose/position.hpp"
#include "rose/score.hpp"
#include "rose/square.hpp"

#include <array>
#include <bit>
#include <tuple>

namespace rose::see {

  inline Score value(PieceType ptype) {
    constexpr std::array<Score, 8> lut {{0, 10000, 100, 300, 0, 300, 500, 900}};
    return lut[ptype.to_index()];
  }

  inline Score gain(const Position& pos, Move mv) {
    if (mv.castle()) {
      return 0;
    }
    if (mv.enpassant()) {
      return value(PieceType::p);
    }

    Score score = value(pos.place_at(mv.to()).ptype());
    if (mv.promo()) {
      score += value(mv.ptype()) - value(PieceType::p);
    }
    return score;
  }

  inline bool see(const Position& pos, Move mv, Score threshold) {
    const Square sq = mv.to();

    Color stm = pos.stm();
    Score score = gain(pos, mv) - threshold;
    if (score < 0) {
      return false;
    }

    score -= value(mv.promo() ? mv.ptype() : pos.place_at(mv.from()).ptype());
    stm = !stm;

    if (score >= 0) {
      return true;
    }

    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);
    const u8x64 ray_places = ray_coords.swizzle(pos.board().to_vector());
    const u8x64 ray_attackers = geometry::attackers_from_rays(ray_places).mask(ray_places);
    const u8x64 ptypes = ray_valid.mask(ray_attackers & u8x64::splat(Place::ptype_mask));

    const u64 color = ray_places.test(u8x64::splat(Place::color_mask)).to_bits();
    const u64 attackers = (ray_attackers.nonzeros() & ray_valid).to_bits();
    u64 occupied = (ray_places.nonzeros() & ray_valid).to_bits();

    // Remove already moved piece and enpassant victim
    occupied ^= ray_coords.eq(u8x64::splat(mv.from().raw)).to_bits();
    if (mv.enpassant()) {
      occupied &= pos.stm() == Color::black ? 0xFFFFFFFFFFFFFFFD : 0xFFFFFFFDFFFFFFFF;
    }

    // Extract bitrays for each piece type
    // Note: Due to Rose's unique ptype ordering, we need to offset the ptype values by 2, so king comes last.
    std::array<u64, 8> ptype_bits {
      ptypes.eq(u8x64::splat(static_cast<u8>(PieceType::p) << Place::ptype_shift)).to_bits(),
      0x0101010101010101,  // Knight
      0,                   // Invalid
      ptypes.eq(u8x64::splat(static_cast<u8>(PieceType::b) << Place::ptype_shift)).to_bits(),
      ptypes.eq(u8x64::splat(static_cast<u8>(PieceType::r) << Place::ptype_shift)).to_bits(),
      ptypes.eq(u8x64::splat(static_cast<u8>(PieceType::q) << Place::ptype_shift)).to_bits(),
      0,  // None
      ptypes.eq(u8x64::splat(static_cast<u8>(PieceType::k) << Place::ptype_shift)).to_bits(),
    };
    u64x8 ptype_vec {ptype_bits};

    auto current_attackers = [&]() {
      return geometry::superpiece_attacks(occupied, occupied) & (~color ^ stm.to_bitboard().raw) & attackers;
    };

    while (const u64 current = current_attackers()) {
      const usize next = static_cast<usize>(std::countr_zero((ptype_vec & u64x8::splat(current)).nonzeros().to_bits()));
      // We need to undo the offset by 2 done earlier.
      const PieceType ptype = PieceType::from_index((next + 2) % 8);
      const u64 br = ptype_bits[next] & current;
      occupied ^= br & -br;

      score = -score - 1 - value(ptype);
      stm = !stm;

      if (score >= 0) {
        if (ptype == PieceType::k && current_attackers() != 0) {
          // We'd be in check if we actually did that.
          stm = !stm;
        }
        break;
      }
    }

    return stm != pos.stm();
  }

}  // namespace rose::see
