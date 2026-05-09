#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/square.hpp"
#include "rose/util/multi_array.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace rose {
  template<i32 max>
  inline auto gravity_formula(i16& value, i32 bonus) -> void {
    bonus = std::clamp(bonus, -max, max);
    value += bonus - std::abs(bonus) * value / max;
  }

  struct QuietHistory {
  private:
    constexpr static i32 entry_max = 8192;

    multi_array<i16, Color::count, Square::count, Square::count> m_table {};

  public:
    auto reset() -> void {
      m_table = {};
    }

    auto update(Color stm, Move mv, i32 bonus) -> void {
      i16& entry = m_table[stm.to_index()][mv.from().to_index()][mv.to().to_index()];
      gravity_formula<entry_max>(entry, bonus);
    }

    auto get(Color stm, Move mv) const -> i32 {
      return m_table[stm.to_index()][mv.from().to_index()][mv.to().to_index()];
    }
  };

}  // namespace rose
