#pragma

#include "rose/hash.h"

#include <array>

#include "rose/util/bch/bch.h"
#include "rose/util/types.h"

namespace rose::hash {

  namespace {
    constexpr usize n = 10;
    const std::vector<bch::Generator<n>> generators = bch::generateGenerators<n>(bch::generateMinPolys<n>());
    const bch::Matrix<n> bm = bch::genBasisMatrix<n>(64, generators);
    const bch::Matrix<n> pm = bch::genParityMatrix<n>(bm);
    inline auto toHash(bch::MatrixRow<n> row) -> u64 { return static_cast<u64>(bch::bitReverse<64>(static_cast<u64>(row))); }
  } // namespace

  const std::array<std::array<u64, 64>, 16> piece_table = [] {
    std::array<std::array<u64, 64>, 16> piece_table{};
    usize i = 0;
    for (usize ptype : {001, 002, 003, 005, 006, 007, 011, 012, 013, 015, 016, 017}) {
      for (usize sq = 0; sq < 64; sq++) {
        piece_table[ptype][sq] = toHash(pm[i++]);
      }
    }
    return piece_table;
  }();

  const std::array<u64, 8> enpassant_table = [] {
    std::array<u64, 8> enpassant_table{};
    usize i = 64 * 12;
    for (usize j = 0; j < 8; j++) {
      enpassant_table[j] = toHash(pm[i++]);
    }
    return enpassant_table;
  }();

  const std::array<u64, 16> castle_table = [] {
    std::array<u64, 4> base{};
    usize i = 64 * 12 + 8;
    base[0] = toHash(pm[i++]);
    base[1] = toHash(pm[i++]);
    base[2] = toHash(pm[i++]);
    base[3] = toHash(pm[i++]);

    std::array<u64, 16> castle_table{};
    for (usize i = 0; i < 16; i++) {
      if (i & 0b0001)
        castle_table[i] ^= base[0];
      if (i & 0b0010)
        castle_table[i] ^= base[1];
      if (i & 0b0100)
        castle_table[i] ^= base[2];
      if (i & 0b1000)
        castle_table[i] ^= base[3];
    }
    return castle_table;
  }();

  const u64 move = toHash(pm[64 * 12 + 8 + 4]);

} // namespace rose::hash
