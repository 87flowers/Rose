#pragma once

#include <algorithm>
#include <limits>

#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose::eval {

  inline constexpr Score no_moves = std::numeric_limits<i16>::min();
  inline constexpr Score min_score = -std::numeric_limits<i16>::max();
  inline constexpr Score max_score = std::numeric_limits<i16>::max();

  inline constexpr i32 max_mate_ply = 256;
  inline constexpr Score min_normal_score = min_score + max_mate_ply + 1;
  inline constexpr Score max_normal_score = max_score - max_mate_ply - 1;

  inline constexpr auto clamp(Score score) -> Score {
    // Clamp to normal score (non-theoretical) range
    return std::clamp(score, min_normal_score, max_normal_score);
  }

  inline constexpr auto isTheoretical(Score score) -> bool {
    rose_assert(score >= min_score && score <= max_score);
    return score < min_normal_score || score > max_normal_score;
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

  inline constexpr auto plysToMate(Score score) -> i32 {
    rose_assert(isTheoretical(score));
    if (score < 0) {
      return score - min_score;
    } else {
      return max_score - score;
    }
  }

  inline constexpr auto adjustPlysToMate(Score score, i32 adjustment) -> Score {
    rose_assert(score >= min_score && score <= max_score);
    if (score < min_normal_score) {
      return eval::mated(eval::plysToMate(score) + adjustment);
    } else if (score > max_normal_score) {
      return eval::mating(eval::plysToMate(score) + adjustment);
    } else {
      return score;
    }
  }

} // namespace rose::eval
