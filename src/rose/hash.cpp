#pragma

#include "rose/hash.h"

#include <array>
#include <random>

#include "rose/util/types.h"

namespace rose::hash {
  const std::array<std::array<u64, 64>, 16> piece_table = [] {
    std::mt19937_64 rand{0x8588A96CAD5E3985};
    std::array<std::array<u64, 64>, 16> piece_table{};
    usize i = 0;
    for (usize ptype = 0; ptype < 16; ptype++) {
      for (usize sq = 0; sq < 64; sq++) {
        piece_table[ptype][sq] = rand();
      }
    }
    return piece_table;
  }();

  const std::array<u64, 8> enpassant_table = [] {
    std::mt19937_64 rand{0x5573B73B2BB36749};
    std::array<u64, 8> enpassant_table{};
    usize i = 64 * 12;
    for (usize j = 0; j < 8; j++) {
      enpassant_table[j] = rand();
    }
    return enpassant_table;
  }();

  const std::array<std::array<u64, 2>, 2> castle_table = [] {
    std::mt19937_64 rand{0x1F78089EF589DECF};
    std::array<std::array<u64, 2>, 2> castle_table{};
    usize i = 64 * 12 + 8;
    castle_table[0][0] = rand();
    castle_table[0][1] = rand();
    castle_table[1][0] = rand();
    castle_table[1][1] = rand();
    return castle_table;
  }();

  const u64 move = 0x51B2ABA9E76DEF82;

} // namespace rose::hash
