#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

#include "rose/square.h"
#include "rose/util/types.h"

namespace rose::rays {

  namespace internal {
    inline constexpr auto genRays(int direction) -> std::array<u64, 8> {
      std::array<u64, 8> rays{};
      u64 bb = std::rotl<u64>(1, direction);
      for (int i = 1; i < 8; i++, bb = std::rotl<u64>(bb, direction)) {
        rays[i] = rays[i - 1] | bb;
      }
      return rays;
    }
  } // namespace internal

  // k = king, c = checker
  inline constexpr auto calcRayTo(Square k, Square c) -> u64 {
    const auto [k_file, k_rank] = k.toFileAndRank();
    const auto [c_file, c_rank] = c.toFileAndRank();
    const int file = c_file - k_file;
    const int rank = c_rank - k_rank;
    const int distance = std::max(std::abs(file), std::abs(rank));
    const int direction = 3 * (1 + (file > 0) - (file < 0)) + (1 + (rank > 0) - (rank < 0));

    // 2 5 8
    // 1 4 7
    // 0 3 6
    constexpr std::array<std::array<u64, 8>, 9> base{{
        internal::genRays(64 - 9),
        internal::genRays(64 - 1),
        internal::genRays(7),
        internal::genRays(64 - 8),
        {},
        internal::genRays(8),
        internal::genRays(64 - 7),
        internal::genRays(1),
        internal::genRays(9),
    }};

    return std::rotl(base[direction][distance], k.raw);
  }

} // namespace rose::rays
