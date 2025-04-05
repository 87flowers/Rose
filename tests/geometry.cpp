#include <print>

#include "rose/byteboard.h"
#include "rose/geometry.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

using namespace rose;

auto testCompressCoords() -> void {
  {
    Byteboard a;
    Byteboard b;
    for (int i = 0; i < 64; i++) {
      a.r[i] = i + (i & 0b111000);
      b.r[i] = i;
    }

    const auto [c, valid] = geometry::internal::compressCoords(a.z);

    rose_assert(c == b.z);
    rose_assert(valid == ~u64{0});
  }

  {
    Byteboard a;
    Byteboard b;
    for (int i = 0; i < 64; i++) {
      a.r[i] = i + (i & 0b111000) | 0x80;
      b.r[i] = i + (i & 0b111000) | 0x08;
    }

    const auto [_, valida] = geometry::internal::compressCoords(a.z);
    const auto [_, validb] = geometry::internal::compressCoords(a.z);

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

  Byteboard p;
  Byteboard b;
  p.z = perm;
  b.z = bperm;

  std::print("perm:\n");
  p.dumpRaw();
  std::print("\n");
  std::print("bperm:\n");
  b.dumpRaw();
  std::print("\n");

  for (int i = 0; i < 64; i++) {
    if ((valid >> i) & 1) {
      rose_assert(b.r[p.r[i]] == i, "{:02x}: b[{:02x}] == {:02x}\n", i, p.r[i], b.r[p.r[i]]);
    }
    if ((bitboard >> i) & 1) {
      rose_assert(p.r[b.r[i]] == i, "{:02x}: p[{:02x}] == {:02x}\n", i, b.r[i], p.r[b.r[i]]);
    }
  }
}

auto main() -> int {
  testCompressCoords();
  testSuperpieceRays();
  return 0;
}
