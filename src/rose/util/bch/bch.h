#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "rose/util/bch/bit.h"
#include "rose/util/bch/polynomial.h"
#include "rose/util/types.h"

namespace rose::bch {
  template <usize n> inline constexpr usize matrix_row_bits = (1 << n) - 1;
  template <usize n> using MatrixRow = unsigned _BitInt(matrix_row_bits<n>);
  template <usize n> using Matrix = std::array<MatrixRow<n>, matrix_row_bits<n>>;

  template <usize n> constexpr auto genBasisMatrix(usize m, std::vector<Generator<n>> generators) -> Matrix<n> {
    std::vector<MatrixRow<n>> use_gs;
    use_gs.push_back(0);
    use_gs.push_back(1);
    for (const auto g : generators) {
      use_gs.push_back(g);
      if (bitWidth(g) >= m)
        break;
    }
    std::reverse(use_gs.begin(), use_gs.end());

    Matrix<n> result;
    usize g_index = 0;
    for (usize j = 0; j < matrix_row_bits<n>; j++) {
      while (j > ctz(bitReverse(use_gs[g_index])))
        g_index++;
      result[j] = bitReverse(use_gs[g_index]) >> j;
    }
    return result;
  }

  template <usize n> constexpr auto genParityMatrix(Matrix<n> bm) -> Matrix<n> {
    auto result = bm;
    for (usize j = 0; j < matrix_row_bits<n>; j++) {
      const usize i = matrix_row_bits<n> - j - 1;
      for (usize k = 0; k < j; k++) {
        const MatrixRow<n> mask = MatrixRow<n>{1} << i;
        const MatrixRow<n> bit = result[k] & mask;
        if (bit != 0)
          result[k] ^= result[j] & ~mask;
      }
    }
    std::reverse(result.begin(), result.end());
    return result;
  }
} // namespace rose::bch
