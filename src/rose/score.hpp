#pragma once

#include "rose/common.hpp"
#include "rose/util/assert.hpp"

#include <algorithm>
#include <limits>

namespace rose {
  using Score = i32;
}  // namespace rose

namespace rose::score {
  inline constexpr Score none = -32768;
  inline constexpr Score infinity = 32767;
  inline constexpr Score min_score = -32766;
  inline constexpr Score max_score = 32766;

  inline constexpr Score max_mate_ply = 256;
  inline constexpr Score min_normal_score = min_score + max_mate_ply + 1;
  inline constexpr Score max_normal_score = max_score - max_mate_ply - 1;

  inline constexpr auto clamp_normal(Score score) -> Score {
    // Clamp to normal score (non-theoretical) range
    return std::clamp(score, min_normal_score, max_normal_score);
  }

  inline constexpr auto is_theoretical(Score score) -> bool {
    return score < min_normal_score || score > max_normal_score;
  }

  inline constexpr auto is_loss(Score score) -> bool {
    return score < min_normal_score;
  }

  inline constexpr auto is_win(Score score) -> bool {
    return score > max_normal_score;
  }

  inline constexpr auto mated(i32 ply) -> Score {
    rose_assert(ply >= 0);
    // If too far away, return min_normal_score
    return std::min(min_score + ply, min_normal_score);
  }

  inline constexpr auto mating(i32 ply) -> Score {
    rose_assert(ply >= 0);
    // If too far away, return max_normal_score
    return std::max(max_score - ply, max_normal_score);
  }

  inline constexpr auto plys_to_mate(Score score) -> i32 {
    rose_assert(is_theoretical(score));
    if (score < 0) {
      return score - min_score;
    } else {
      return max_score - score;
    }
  }

  inline constexpr auto adjust_plys_to_mate(Score score, i32 adjustment) -> Score {
    rose_assert(score >= min_score && score <= max_score);
    if (score < min_normal_score) {
      return score::mated(std::max(0, score::plys_to_mate(score) + adjustment));
    } else if (score > max_normal_score) {
      return score::mating(std::max(0, score::plys_to_mate(score) + adjustment));
    } else {
      return score;
    }
  }
}  // namespace rose::score
