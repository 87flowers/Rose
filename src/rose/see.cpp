#include "rose/see.h"

#include <bit>
#include <print>

#include "rose/common.h"
#include "rose/geometry.h"
#include "rose/move.h"
#include "rose/position.h"

namespace rose::see {

  static auto value(PieceType ptype) -> i32 {
    constexpr std::array<i32, 8> lut{{0, 10000, 100, 300, 0, 300, 500, 900}};
    return lut[ptype.toIndex()];
  }

  auto see(const Position &position, Move move, i32 threshold) -> bool {
    if (move.castle())
      return 0 >= threshold;

    const Square from = move.from();
    const Square to = move.to();

    Color stm = position.activeColor();

    i32 score = value(move.enpassant() ? PieceType::p : position.pieceOn(to));
    if (move.promo()) {
      score += value(move.ptype()) - value(PieceType::p);
    }
    score -= threshold;

    if (score < 0)
      return false;

    PieceType next = move.promo() ? move.ptype() : position.pieceOn(from);
    score -= value(next);
    stm = stm.invert();

    if (score >= 0)
      return true;

    // Remove already moved piece from board.
    const auto board = vec::mask8(~from.toBitboard(), position.board().z);

    const auto [ray_coords, ray_valid] = geometry::superpieceRays(to);
    const v512 ray_places = vec::permute8(ray_coords, board);
    const u64 attackers = ray_valid & geometry::attackersFromRays(ray_places);
    const u64 color = ray_places.msb8();

    u64 occupied = ray_places.nonzero8() & ray_valid;

    // Remove enpassant victim
    if (move.enpassant()) {
      occupied &= position.activeColor() == Color::black ? 0xFFFFFFFFFFFFFFFD : 0xFFFFFFFDFFFFFFFF;
    }

    // Extract bitrays for each piece type
    // Note: Due to Rose's unique ptype ordering, we need to offset the ptype values by 2, so king comes last.
    const v512 bitptype =
        vec::permute8(vec::mask8(attackers, vec::shr16(ray_places, 4) & v512::broadcast8(0x0F)),
                      v128{std::array<u8, 16>{0x00, 0x80, 0x01, 0x02, 0x00, 0x08, 0x10, 0x20, 0x00, 0x80, 0x01, 0x02, 0x00, 0x08, 0x10, 0x20}});
    const v512 piece_rays_vec =
        vec::permute8(v512{std::array<u8, 64>{0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x01, 0x09, 0x11, 0x19, 0x21, 0x29, 0x31, 0x39,
                                              0x02, 0x0A, 0x12, 0x1A, 0x22, 0x2A, 0x32, 0x3A, 0x03, 0x0B, 0x13, 0x1B, 0x23, 0x2B, 0x33, 0x3B,
                                              0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C, 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D,
                                              0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E, 0x07, 0x0F, 0x17, 0x1F, 0x27, 0x2F, 0x37, 0x3F}},
                      vec::gf2p8matmul8(vec::gf2p8matmul8(v512::broadcast64(0x8040201008040201), bitptype), v512::broadcast64(0x8040201008040201)));
    auto piece_rays = std::bit_cast<std::array<u64, 8>>(piece_rays_vec);

    auto current_attackers = [&]() { return geometry::superpieceAttacks(occupied, occupied) & attackers & (~color ^ stm.toBitboard()); };

    while (u64 current = current_attackers()) {
      int next = std::countr_zero((piece_rays_vec & v512::broadcast64(current)).nonzero64());
      // We need to undo the offset by 2 done earlier.
      PieceType ptype = static_cast<PieceType::Inner>((next + 2) % 8);
      u64 br = piece_rays[next] & current;
      occupied ^= br & -br;

      score = -score - 1 - value(ptype);
      stm = stm.invert();

      if (ptype == PieceType::k) {
        if (current_attackers() != 0)
          stm = stm.invert();
        break;
      }

      if (score >= 0)
        break;
    }

    return stm != position.activeColor();
  }

} // namespace rose::see
