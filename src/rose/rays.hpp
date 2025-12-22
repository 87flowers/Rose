#pragma once

#include "rose/bitboard.hpp"
#include "rose/common.hpp"
#include "rose/square.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

namespace rose::rays {

  namespace internal {
    inline constexpr auto dir_bb(int direction) -> u64 {
      return std::rotl<u64>(1, direction);
    }

    inline constexpr auto gen_rays(int direction) -> std::array<u64, 8> {
      std::array<u64, 8> rays {};
      u64 bb = dir_bb(direction);
      for (int i = 1; i < 8; i++, bb = std::rotl<u64>(bb, direction)) {
        rays[i] = rays[i - 1] | bb;
      }
      return rays;
    }
  }  // namespace internal

  // k = king, c = checker
  inline constexpr auto calc_ray_to(Square k, Square c) -> Bitboard {
    const auto [k_file, k_rank] = k.to_file_and_rank();
    const auto [c_file, c_rank] = c.to_file_and_rank();
    const int file = c_file - k_file;
    const int rank = c_rank - k_rank;
    const int distance = std::max(std::abs(file), std::abs(rank));
    const int direction = 3 * (1 + (file > 0) - (file < 0)) + (1 + (rank > 0) - (rank < 0));

    // 2 5 8
    // 1 4 7
    // 0 3 6
    constexpr std::array<std::array<u64, 8>, 9> base {{
      internal::gen_rays(64 - 9),
      internal::gen_rays(64 - 1),
      internal::gen_rays(7),
      internal::gen_rays(64 - 8),
      {},
      internal::gen_rays(8),
      internal::gen_rays(64 - 7),
      internal::gen_rays(1),
      internal::gen_rays(9),
    }};

    return Bitboard {std::rotl(base[direction][distance], k.raw)};
  }

  inline constexpr auto king_invalid_destinations(Square k, Square c) -> Bitboard {
    const auto [k_file, k_rank] = k.to_file_and_rank();
    const auto [c_file, c_rank] = c.to_file_and_rank();
    const int file = c_file - k_file;
    const int rank = c_rank - k_rank;
    const int direction = 3 * (1 + (file > 0) - (file < 0)) + (1 + (rank > 0) - (rank < 0));

    // 2 5 8
    // 1 4 7
    // 0 3 6
    constexpr std::array<u64, 9> base {{
      internal::dir_bb(64 - 9) | internal::dir_bb(9),
      internal::dir_bb(64 - 1) | internal::dir_bb(1),
      internal::dir_bb(7) | internal::dir_bb(64 - 7),
      internal::dir_bb(64 - 8) | internal::dir_bb(8),
      0,
      internal::dir_bb(8) | internal::dir_bb(64 - 8),
      internal::dir_bb(64 - 7) | internal::dir_bb(7),
      internal::dir_bb(1) | internal::dir_bb(64 - 1),
      internal::dir_bb(9) | internal::dir_bb(64 - 9),
    }};

    return Bitboard {std::rotl(base[direction], k.raw)};
  }

}  // namespace rose::rays
