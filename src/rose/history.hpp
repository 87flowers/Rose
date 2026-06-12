#pragma once

#include "rose/bitboard.hpp"
#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/square.hpp"
#include "rose/util/multi_array.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <initializer_list>

namespace rose {
  inline constexpr std::initializer_list<i32> conthists_indexes {1, 2, 4, 6};

  template<i32 max>
  inline auto gravity_formula(i16& value, i32 bonus) -> void {
    bonus = std::clamp(bonus, -max, max);
    value += bonus - std::abs(bonus) * value / max;
  }

  struct QuietHistory {
  private:
    constexpr static i32 entry_max = 8192;

    multi_array<i16, Color::count, Square::count, Square::count, 2, 2> m_table {};

  public:
    auto reset() -> void {
      m_table = {};
    }

    auto update(Color stm, Bitboard threats, Move mv, i32 bonus) -> void {
      i16& entry = m_table[stm.to_index()][mv.from().to_index()][mv.to().to_index()][threats.read(mv.from())][threats.read(mv.to())];
      gravity_formula<entry_max>(entry, bonus);
    }

    auto get(Color stm, Bitboard threats, Move mv) const -> i32 {
      return m_table[stm.to_index()][mv.from().to_index()][mv.to().to_index()][threats.read(mv.from())][threats.read(mv.to())];
    }
  };

  struct NoisyHistory {
  private:
    constexpr static i32 entry_max = 8192;

    multi_array<i16, Color::count, PieceType::count, Square::count, Square::count> m_table {};

  public:
    auto reset() -> void {
      m_table = {};
    }

    auto update(Color stm, PieceType attacker, Move mv, i32 bonus) -> void {
      i16& entry = m_table[stm.to_index()][attacker.to_index()][mv.from().to_index()][mv.to().to_index()];
      gravity_formula<entry_max>(entry, bonus);
    }

    auto get(Color stm, PieceType attacker, Move mv) const -> i32 {
      return m_table[stm.to_index()][attacker.to_index()][mv.from().to_index()][mv.to().to_index()];
    }
  };

  struct ContinuationHistorySubtable {
  private:
    constexpr static i32 entry_max = 8192;
    multi_array<i16, Color::count, PieceType::count, Square::count> m_table {};

  public:
    auto reset() -> void {
      m_table = {};
    }

    auto update(Color stm, PieceType ptype, Move mv, i32 bonus) -> void {
      i16& entry = m_table[stm.to_index()][ptype.to_index()][mv.to().to_index()];
      gravity_formula<entry_max>(entry, bonus);
    }

    auto get(Color stm, PieceType ptype, Move mv) const -> i32 {
      return m_table[stm.to_index()][ptype.to_index()][mv.to().to_index()];
    }
  };

  struct ContinuationHistory {
  private:
    multi_array<ContinuationHistorySubtable, Color::count, 2, PieceType::count, Square::count> m_table {};

  public:
    auto reset() -> void {
      m_table = {};
    }

    auto get_subtable(Color stm, bool capture, PieceType ptype, Move mv) -> ContinuationHistorySubtable* {
      return &m_table[stm.to_index()][capture][ptype.to_index()][mv.to().to_index()];
    }
  };

}  // namespace rose
