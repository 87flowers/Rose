#include <print>

#include "rose/byteboard.h"
#include "rose/geometry.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

using namespace rose;

auto testCompressCoords() -> void {
  {
    std::array<u8, 64> a;
    std::array<u8, 64> b;
    for (int i = 0; i < 64; i++) {
      a[i] = i + (i & 0b111000);
      b[i] = i;
    }

    const auto [c, valid] = geometry::internal::compressCoords(std::bit_cast<v512>(a));

    rose_assert(c == std::bit_cast<v512>(b));
    rose_assert(valid == ~u64{0});
  }

  {
    std::array<u8, 64> a;
    std::array<u8, 64> b;
    for (int i = 0; i < 64; i++) {
      a[i] = i + (i & 0b111000) | 0x80;
      b[i] = i + (i & 0b111000) | 0x08;
    }

    const auto [_, valida] = geometry::internal::compressCoords(std::bit_cast<v512>(a));
    const auto [_, validb] = geometry::internal::compressCoords(std::bit_cast<v512>(b));

    rose_assert(valida == 0);
    rose_assert(validb == 0);
  }
}

auto testSuperpieceRays() -> void {
  const Square sq = Square::parse("e4").value();
  const auto [perm, valid] = geometry::superpieceRays(sq);
  const auto bperm = geometry::superpieceInverseRays(sq);
  const auto bitboard = ~bperm.msb8();

  rose_assert(bitboard == 0b0001000110010010011111000111110011101111011111000111110010010010);

  const auto p = std::bit_cast<std::array<u8, 64>>(perm);
  const auto b = std::bit_cast<std::array<u8, 64>>(bperm);

  for (int i = 0; i < 64; i++) {
    if ((valid >> i) & 1) {
      rose_assert(b[p[i]] == i, "{:02x}: b[{:02x}] == {:02x}\n", i, p[i], b[p[i]]);
    }
    if ((bitboard >> i) & 1) {
      rose_assert(p[b[i]] == i, "{:02x}: p[{:02x}] == {:02x}\n", i, b[i], p[b[i]]);
    }
  }
}

auto main() -> int {
  testCompressCoords();
  testSuperpieceRays();
  return 0;
}
