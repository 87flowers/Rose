#include <array>
#include <bit>
#include <print>

#include "rose/geometry.h"
#include "rose/move.h"
#include "rose/position.h"
#include "rose/square.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

using namespace rose;

// 3r3k/3r4/2n1n3/8/3p4/2PR4/1B1Q4/3R3K w - - | d3d4 | -100 | P - R + N - P + N - B + R - Q + R

template <typename T> auto print8(T x) -> void {
  for (u8 x : std::bit_cast<std::array<u8, sizeof(x) / sizeof(u8)>>(x)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");
}

template <typename T> auto print64(T x) -> void {
  for (u64 x : std::bit_cast<std::array<u64, sizeof(x) / sizeof(u64)>>(x)) {
    std::print("{:016x} ", x);
  }
  std::print("\n");
}

auto oldSee(Position position, const Move move) -> void {
  const Square sq = move.to();

  position.prettyPrint();

  position = position.move(move);

  position.prettyPrint();

  const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
  const v512 ray_places = vec::permute8(ray_coords, position.board().z);
  const u64 attackers = ray_valid & geometry::attackersFromRays(ray_places);
  const u64 occupied = ray_places.nonzero8();
  const u64 color = ray_places.msb8();

  const v512 bitptype =
      vec::permute8(vec::mask8(attackers, vec::shr16(ray_places, 4)),
                    v128{std::array<u8, 16>{0x00, 0x20, 0x01, 0x02, 0x00, 0x04, 0x08, 0x10, 0x00, 0x20, 0x01, 0x02, 0x00, 0x04, 0x08, 0x10}});
  print8(bitptype);

  const v512 ptypevec =
      vec::permute8(v512{std::array<u8, 64>{0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x01, 0x09, 0x11, 0x19, 0x21, 0x29, 0x31, 0x39,
                                            0x02, 0x0A, 0x12, 0x1A, 0x22, 0x2A, 0x32, 0x3A, 0x03, 0x0B, 0x13, 0x1B, 0x23, 0x2B, 0x33, 0x3B,
                                            0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C, 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D,
                                            0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E, 0x07, 0x0F, 0x17, 0x1F, 0x27, 0x2F, 0x37, 0x3F}},
                    vec::gf2p8matmul8(vec::gf2p8matmul8(v512::broadcast64(0x8040201008040201), bitptype), v512::broadcast64(0x8040201008040201)));
  print64(ptypevec);
}

auto newSee(Position position, const Move move) -> void {
  const Square sq = move.to();

  position.prettyPrint();

  position = position.move(move);

  position.prettyPrint();

  const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
  const v512 ray_places = vec::permute8(ray_coords, position.board().z);

  const u64 occupied = ray_places.nonzero8();
  const u64 color = ray_places.msb8();

  const u64 queens = vec::eq8(ray_places & v512::broadcast8(0x70), v512::broadcast8(narrow_cast<u8>(PieceType::q << 4)));
  const u64 attackers = geometry::attackersFromRays(ray_places);

  const u64 rays_to_blockers = geometry::superpieceAttacks(occupied & ~attackers, ray_valid);
  const u64 rays_to_queens = geometry::superpieceAttacks(queens, ray_valid);
  const u64 beyond_queens = rays_to_blockers & ~rays_to_queens;

  const v512 ptype = vec::shr16(ray_places & v512::broadcast8(0x70), 4) | vec::mask8(beyond_queens, v512::broadcast8(0x08));

  v128 white_ptype = vec::compress8(rays_to_blockers & attackers & ~color, ptype).to128();
  v128 black_ptype = vec::compress8(rays_to_blockers & attackers & color, ptype).to128();

  std::print("ptype:\n");
  std::print("w: ");
  print8(white_ptype);
  std::print("b: ");
  print8(black_ptype);

  constexpr v128 ptype_to_bits = v128{std::array<u8, 16>{
      0x00,
      0x80, // king
      0x01, // pawn
      0x02, // knight
      0x00,
      0x04, // bishop
      0x08, // rook
      0x10, // queen
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x20, // bishop behind queen
      0x40, // rook behind queen
      0x00,
  }};
  white_ptype = vec::permute8(white_ptype, ptype_to_bits);
  black_ptype = vec::permute8(black_ptype, ptype_to_bits);

  std::print("ptype (one-hot):\n");
  std::print("w: ");
  print8(white_ptype);
  std::print("b: ");
  print8(black_ptype);

  const v128 transpose = v128::broadcast64(0x8040201008040201);
  white_ptype = vec::gf2p8matmul8(transpose, white_ptype);
  black_ptype = vec::gf2p8matmul8(transpose, black_ptype);

  std::print("counts (number of hot bits):\n");
  std::print("w: ");
  print8(white_ptype);
  std::print("b: ");
  print8(black_ptype);
  std::print("    p  n  b  r  q qb qr  k\n");

  constexpr v512 pvalues{std::array<u8, 64>{
      1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5,   5,   5,   5,   5,   5,   5,   5,
      9, 9, 9, 9, 9, 9, 9, 9, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 128, 128, 128, 128, 128, 128, 128, 128,
  }};
  const v128 white_pvalues = vec::compress8(white_ptype.to64(), pvalues).to128();
  const v128 black_pvalues = vec::compress8(black_ptype.to64(), pvalues).to128();

  std::print("pvalues:\n");
  std::print("w: ");
  print8(white_pvalues);
  std::print("b: ");
  print8(black_pvalues);

  v128 white_psum = white_pvalues;
  v128 black_psum = black_pvalues;

  white_psum = vec::add8_sat(white_psum, vec::lanebyteshl<1>(white_psum));
  black_psum = vec::add8_sat(black_psum, vec::lanebyteshl<1>(black_psum));
  white_psum = vec::add8_sat(white_psum, vec::lanebyteshl<2>(white_psum));
  black_psum = vec::add8_sat(black_psum, vec::lanebyteshl<2>(black_psum));
  white_psum = vec::add8_sat(white_psum, vec::lanebyteshl<4>(white_psum));
  black_psum = vec::add8_sat(black_psum, vec::lanebyteshl<4>(black_psum));
  white_psum = vec::add8_sat(white_psum, vec::lanebyteshl<8>(white_psum));
  black_psum = vec::add8_sat(black_psum, vec::lanebyteshl<8>(black_psum));

  std::print("psum:\n");
  std::print("w: ");
  print8(white_psum);
  std::print("b: ");
  print8(black_psum);

  const u8 value_offset = 4;

  black_psum = vec::add8_sat(black_psum, v128::broadcast8(value_offset));

  std::print("psum(subtracted):\n");
  std::print("w: ");
  print8(vec::sub8_sat(white_psum, black_psum));
  std::print("b: ");
  print8(vec::sub8_sat(black_psum, white_psum));
}

auto main() -> int {
  const Position position = Position::parse("3r3k/3r4/2n1n3/8/3p4/2PR4/1B1Q4/3R3K w - - 0 1").value();
  const Move move = Move::parse("d3d4", position).value();

  oldSee(position, move);
  newSee(position, move);
}
