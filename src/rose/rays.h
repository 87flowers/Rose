#pragma once

#include <array>

#include "rose/square.h"
#include "rose/util/types.h"

namespace rose::rays {

  namespace internal {

    using Table = std::array<std::array<u64, 64>, 64>;

    constexpr int sign(int x) {
      if (x < 0)
        return -1;
      if (x > 0)
        return 1;
      return 0;
    }

    template <typename Hit, typename Miss> consteval Table generate_rays(Hit hit, Miss miss) {
      Table result{};

      for (u8 a_raw = 0; a_raw < 64; a_raw++) {
        for (u8 b_raw = 0; b_raw < 64; b_raw++) {
          Square a{a_raw};
          Square b{b_raw};

          if (a == b) {
            result[a_raw][b_raw] = miss(a, b);
            continue;
          }

          auto [b_file, b_rank] = b.toFileAndRank();
          auto [a_file, a_rank] = a.toFileAndRank();
          int file_diff = b_file - a_file;
          int rank_diff = b_rank - a_rank;

          if (file_diff == 0 || rank_diff == 0 || file_diff == rank_diff || file_diff == -rank_diff) {
            int file_delta = sign(file_diff);
            int rank_delta = sign(rank_diff);

            result[a_raw][b_raw] = hit(a, b, rank_delta, file_delta);
          } else {
            result[a_raw][b_raw] = miss(a, b);
          }
        }
      }

      return result;
    }

    inline constexpr Table inclusive_table = generate_rays(
        [](Square a, Square b, int rank_delta, int file_delta) {
          u64 bb = 0;
          for (auto [file, rank] = a.toFileAndRank(); file != b.file() || rank != b.rank(); file += file_delta, rank += rank_delta)
            bb |= Square::fromFileAndRank(file, rank).toBitboard();
          bb |= a.toBitboard() | b.toBitboard();
          return bb;
        },
        [](Square a, Square b) { return a.toBitboard() | b.toBitboard(); });

    inline constexpr Table infinite_exclusive_table = generate_rays(
        [](Square a, Square b, int rank_delta, int file_delta) {
          u64 bb = 0;
          for (auto [file, rank] = a.toFileAndRank(); file >= 0 && file <= 7 && rank >= 0 && rank <= 7; file += file_delta, rank += rank_delta)
            bb |= Square::fromFileAndRank(file, rank).toBitboard();
          for (auto [file, rank] = a.toFileAndRank(); file >= 0 && file <= 7 && rank >= 0 && rank <= 7; file -= file_delta, rank -= rank_delta)
            bb |= Square::fromFileAndRank(file, rank).toBitboard();
          bb &= ~a.toBitboard() & ~b.toBitboard();
          return bb;
        },
        [](Square, Square) { return 0; });

  } // namespace internal

  constexpr u64 inclusive(Square a, Square b) { return internal::inclusive_table[a.raw][b.raw]; }

  constexpr u64 infinite_exclusive(Square a, Square b) { return internal::infinite_exclusive_table[a.raw][b.raw]; }

} // namespace rose::rays
