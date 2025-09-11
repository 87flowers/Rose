#include <array>
#include <bit>
#include <print>

#include "rose/geometry.h"
#include "rose/square.h"
#include "rose/util/vec.h"

using namespace rose;

auto expandSq(Square sq) -> u8 { return sq.raw + (sq.raw & 0b111000); }
auto expandSq(v128 list) -> v128 { return vec::add8(list, (list & v128::broadcast8(0b111000))); }
auto bitselect(v128 index, v128 a, v128 b) -> v128 { return _mm_ternarylogic_epi32(index.raw, a.raw, b.raw, 0xAC); }

auto main() -> int {
  const Square sq = Square::parse("d4").value();
  const v128 coords{std::array<u8, 16>{
      Square::parse("d6").value().raw,
      Square::parse("f6").value().raw,
      Square::parse("h4").value().raw,
      Square::parse("e3").value().raw,
      Square::parse("d2").value().raw,
      Square::parse("a1").value().raw,
      Square::parse("b4").value().raw,
      Square::parse("a7").value().raw,
      Square::parse("e7").value().raw,
      Square::parse("f5").value().raw,
      Square::parse("h3").value().raw,
      Square::parse("e2").value().raw,
      Square::parse("b1").value().raw,
      Square::parse("a2").value().raw,
      Square::parse("a6").value().raw,
      Square::parse("b8").value().raw,
  }};

  const v128 expandedCoords = vec::gf2p8matmul8(coords, v128::broadcast64(0x0102040008102000));
  const v128 expandedSq = v128::broadcast8(expandSq(sq));

  const v128 diff = vec::sub8(expandedCoords | v128::broadcast8(0x88), expandedSq);
  const v128 zero = vec::add8(expandedCoords ^ expandedSq, v128::broadcast8(0x77));

  const v128 nearly_indexes =
      vec::gf2p8matmul8(diff, v128::broadcast64(0x8008000000000000)) | vec::gf2p8matmul8(zero, v128::broadcast64(0x0000800800000000));

  const v128 indexes =
      vec::permute8(nearly_indexes, v128{std::array<u8, 16>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 4, 0, 0xFF, 6, 0xFF, 2, 5, 7, 3, 1}});

  for (u8 x : std::bit_cast<std::array<u8, 16>>(expandedCoords)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");

  for (u8 x : std::bit_cast<std::array<u8, 16>>(expandedSq)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");

  for (u8 x : std::bit_cast<std::array<u8, 16>>(diff)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");

  for (u8 x : std::bit_cast<std::array<u8, 16>>(zero)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");

  for (u8 x : std::bit_cast<std::array<u8, 16>>(nearly_indexes)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");

  for (u8 x : std::bit_cast<std::array<u8, 16>>(indexes)) {
    std::print("{:02x} ", x);
  }
  std::print("\n");

  return 0;
}
