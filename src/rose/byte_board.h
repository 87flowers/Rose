#pragma once

#include <array>
#include <print>

#include "rose/common.h"
#include "rose/square.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  union alignas(64) Byteboard {
    std::array<v512, 2> z;
    std::array<Place, 128> m;
    std::array<u8, 128> r{};

    constexpr Byteboard() = default;

    auto dumpRaw() const -> void {
      for (int flipped_rank = 0; flipped_rank < 8; flipped_rank++) {
        for (int file = 0; file < 8; file++) {
          const int rank = 7 - flipped_rank;
          const Square sq = Square::fromFileAndRank(file, rank);
          std::print("{:02x} ", r[sq.raw]);
        }
        std::print("\n");
      }
    }
  };

} // namespace rose
