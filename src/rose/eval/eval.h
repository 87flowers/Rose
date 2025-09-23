#pragma once

#include <algorithm>
#include <limits>

#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose::eval {

  inline constexpr i32 no_moves = std::numeric_limits<i16>::min();
  inline constexpr i32 infinity = -no_moves;
  inline constexpr i32 min_score = -std::numeric_limits<i16>::max();
  inline constexpr i32 max_score = std::numeric_limits<i16>::max();

  inline constexpr i32 max_mate_ply = 256;
  inline constexpr i32 min_normal_score = min_score + max_mate_ply + 1;
  inline constexpr i32 max_normal_score = max_score - max_mate_ply - 1;

  inline constexpr auto clamp(i32 score) -> i32 {
    // Clamp to normal score (non-theoretical) range
    return std::clamp(score, min_normal_score, max_normal_score);
  }

  inline constexpr auto isTheoretical(i32 score) -> bool { return score < min_normal_score || score > max_normal_score; }
  inline constexpr auto isLoss(i32 score) -> bool { return score < min_normal_score; }
  inline constexpr auto isWin(i32 score) -> bool { return score > max_normal_score; }

  inline constexpr auto mated(i32 ply) -> i32 {
    rose_assert(ply >= 0);
    // If too far away, return min_normal_score
    return std::min(min_score + ply, min_normal_score);
  }

  inline constexpr auto mating(i32 ply) -> i32 {
    rose_assert(ply >= 0);
    // If too far away, return max_normal_score
    return std::max(max_score - ply, max_normal_score);
  }

  inline constexpr auto plysToMate(i32 score) -> i32 {
    rose_assert(isTheoretical(score));
    if (score < 0) {
      return score - min_score;
    } else {
      return max_score - score;
    }
  }

  inline constexpr auto adjustPlysToMate(i32 score, i32 adjustment) -> i32 {
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
