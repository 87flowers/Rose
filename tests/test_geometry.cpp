#include "rose/board.hpp"
#include "rose/common.hpp"
#include "rose/geometry.hpp"
#include "rose/util/assert.hpp"

#include <fmt/format.h>

using namespace rose;

auto test_compress_coords() -> void {
  {
    std::array<u8, 64> a;
    std::array<u8, 64> b;
    for (int i = 0; i < 64; i++) {
      a[i] = i + (i & 0b111000);
      b[i] = i;
    }

    const auto [c, valid] = geometry::internal::compress_coords(std::bit_cast<u8x64>(a));

    rose_assert(c == std::bit_cast<u8x64>(b));
    rose_assert(valid.to_bits() == ~u64 {0});
  }

  {
    std::array<u8, 64> a;
    std::array<u8, 64> b;
    for (int i = 0; i < 64; i++) {
      a[i] = i + (i & 0b111000) | 0x80;
      b[i] = i + (i & 0b111000) | 0x08;
    }

    const auto [_, valida] = geometry::internal::compress_coords(std::bit_cast<u8x64>(a));
    const auto [_, validb] = geometry::internal::compress_coords(std::bit_cast<u8x64>(b));

    rose_assert(valida.to_bits() == 0);
    rose_assert(validb.to_bits() == 0);
  }
}

auto test_superpiece_rays() -> void {
  const Square sq = Square::parse("e4").value();
  const auto [perm, valid] = geometry::superpiece_rays(sq);
  const auto bperm = geometry::superpiece_inverse_rays(sq);
  const auto bitboard = ~bperm.msb().to_bits();

  const auto p = std::bit_cast<std::array<u8, 64>>(perm);
  const auto b = std::bit_cast<std::array<u8, 64>>(bperm);

  for (int i = 0; i < 64; i++) {
    fmt::print("{:02x} ", p[i]);
  }
  fmt::print("\n");
  for (int i = 0; i < 64; i++) {
    fmt::print("{:02x} ", b[i]);
  }
  fmt::print("\n");

  for (int i = 0; i < 64; i++) {
    if ((valid.to_bits() >> i) & 1) {
      rose_assert(b[p[i]] == i, "{:02x}: b[{:02x}] == {:02x}\n", i, p[i], b[p[i]]);
    }
    if ((bitboard >> i) & 1) {
      rose_assert(p[b[i]] == i, "{:02x}: p[{:02x}] == {:02x}\n", i, b[i], p[b[i]]);
    }
  }

  rose_assert(bitboard == 0b0001000110010010011111000111110011101111011111000111110010010010);
}

auto main() -> int {
  test_compress_coords();
  test_superpiece_rays();
  return 0;
}
