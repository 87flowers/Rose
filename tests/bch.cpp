#include <algorithm>
#include <print>

#include "rose/util/assert.h"
#include "rose/util/bch/bch.h"
#include "rose/util/bch/polynomial.h"

using namespace rose;
using namespace rose::bch;

auto testPow() -> void {
  const Gf2<3> a{0b010};
  Gf2<3> value{0b001};
  rose_assert(0b001 == value.raw);
  rose_assert(0b001 == a.pow(0).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b010 == value.raw);
  rose_assert(0b010 == a.pow(1).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b100 == value.raw);
  rose_assert(0b100 == a.pow(2).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b011 == value.raw);
  rose_assert(0b011 == a.pow(3).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b110 == value.raw);
  rose_assert(0b110 == a.pow(4).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b111 == value.raw);
  rose_assert(0b111 == a.pow(5).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b101 == value.raw);
  rose_assert(0b101 == a.pow(6).raw);
  value = Gf2<3>::mul(value, a);
  rose_assert(0b001 == value.raw);
  rose_assert(0b001 == a.pow(7).raw);
}

auto testFindMinPoly() -> void {
  rose_assert(0b0011 == Gf2<3>{0b001}.findMinPoly());
  rose_assert(0b1011 == Gf2<3>{0b010}.findMinPoly());
  rose_assert(0b1101 == Gf2<3>{0b011}.findMinPoly());
}

auto testFive() -> void {
  const auto min5 = generateMinPolys<5>();
  rose_assert(min5.size() == 6);
  rose_assert(min5[0] == 0b100101);
  rose_assert(min5[1] == 0b111101);
  rose_assert(min5[2] == 0b110111);
  rose_assert(min5[3] == 0b101111);
  rose_assert(min5[4] == 0b111011);
  rose_assert(min5[5] == 0b101001);

  const auto gen5 = generateGenerators<5>(min5);
  rose_assert(gen5.size() == 6);
  rose_assert(gen5[0] == 0b100101);
  rose_assert(gen5[1] == 0b11101101001);
  rose_assert(gen5[2] == 0b1000111110101111);
  rose_assert(gen5[3] == 0b101100010011011010101);
  rose_assert(gen5[4] == 0b11001011011110101000100111);
  rose_assert(gen5[5] == 0b1111111111111111111111111111111);
}

auto testGenParityMatrix() -> void {
  auto min_polys = generateMinPolys<4>();
  min_polys.push_back(0b11);
  std::rotate(min_polys.begin(), min_polys.begin() + 3, min_polys.end());
  const auto generators = generateGenerators<4>(min_polys);

  const auto bm = genBasisMatrix<4>(8, generators);
  const auto pm = genParityMatrix<4>(bm);

  for (auto b : bm)
    std::print("{:x}\n", static_cast<u64>(b));
  for (auto p : pm)
    std::print("{:x}\n", static_cast<u64>(p));

  rose_assert((bm == Matrix<4>{
                         0b101100110100000,
                         0b010110011010000,
                         0b001011001101000,
                         0b000101100110100,
                         0b000010110011010,
                         0b000001011001101,
                         0b000000110101000,
                         0b000000011010100,
                         0b000000001101010,
                         0b000000000110101,
                         0b000000000010011,
                         0b000000000001000,
                         0b000000000000100,
                         0b000000000000010,
                         0b000000000000001,
                     }));
  rose_assert((pm == Matrix<4>{
                         0b000000000000001,
                         0b000000000000010,
                         0b000000000000100,
                         0b000000000001000,
                         0b000000000010011,
                         0b000000000110110,
                         0b000000001111100,
                         0b000000011101011,
                         0b000000111010101,
                         0b000001010011010,
                         0b000010100100111,
                         0b000101101101110,
                         0b001011111111111,
                         0b010111011001101,
                         0b101110110011001,
                     }));
}

auto main() -> int {
  testPow();
  testFive();
  testGenParityMatrix();
  return 0;
}
