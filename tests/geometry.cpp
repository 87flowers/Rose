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
  {
    const Square sq = Square::parse("e4").value();
    const auto [z, valid] = geometry::superpieceRays(sq);
    Byteboard bb;
    bb.z = z;
    rose_assert(valid == 0b1111111100001110001111000111100001110000111000011100011110000111);
    rose_assert(bb.r[0] == Square::parse("f5").value().raw);
    rose_assert(bb.r[1] == Square::parse("g6").value().raw);
    rose_assert(bb.r[2] == Square::parse("h7").value().raw);
    rose_assert(bb.r[7] == Square::parse("d5").value().raw);
    rose_assert(bb.r[8] == Square::parse("c6").value().raw);
    rose_assert(bb.r[9] == Square::parse("b7").value().raw);
    rose_assert(bb.r[10] == Square::parse("a8").value().raw);
    rose_assert(bb.r[14] == Square::parse("f3").value().raw);
    rose_assert(bb.r[15] == Square::parse("g2").value().raw);
    rose_assert(bb.r[16] == Square::parse("h1").value().raw);
    rose_assert(bb.r[21] == Square::parse("d3").value().raw);
    rose_assert(bb.r[22] == Square::parse("c2").value().raw);
    rose_assert(bb.r[23] == Square::parse("b1").value().raw);
    rose_assert(bb.r[28] == Square::parse("f4").value().raw);
    rose_assert(bb.r[29] == Square::parse("g4").value().raw);
    rose_assert(bb.r[30] == Square::parse("h4").value().raw);
    rose_assert(bb.r[35] == Square::parse("e5").value().raw);
    rose_assert(bb.r[36] == Square::parse("e6").value().raw);
    rose_assert(bb.r[37] == Square::parse("e7").value().raw);
    rose_assert(bb.r[38] == Square::parse("e8").value().raw);
    rose_assert(bb.r[42] == Square::parse("d4").value().raw);
    rose_assert(bb.r[43] == Square::parse("c4").value().raw);
    rose_assert(bb.r[44] == Square::parse("b4").value().raw);
    rose_assert(bb.r[45] == Square::parse("a4").value().raw);
    rose_assert(bb.r[49] == Square::parse("e3").value().raw);
    rose_assert(bb.r[50] == Square::parse("e2").value().raw);
    rose_assert(bb.r[51] == Square::parse("e1").value().raw);
    rose_assert(bb.r[56] == Square::parse("d2").value().raw);
    rose_assert(bb.r[57] == Square::parse("f2").value().raw);
    rose_assert(bb.r[58] == Square::parse("c3").value().raw);
    rose_assert(bb.r[59] == Square::parse("c5").value().raw);
    rose_assert(bb.r[60] == Square::parse("g3").value().raw);
    rose_assert(bb.r[61] == Square::parse("g5").value().raw);
    rose_assert(bb.r[62] == Square::parse("d6").value().raw);
    rose_assert(bb.r[63] == Square::parse("f6").value().raw);
  }
}

auto main() -> int {
  testCompressCoords();
  testSuperpieceRays();
  return 0;
}
