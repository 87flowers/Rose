#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/movegen.hpp"

#include <span>

namespace rose {

  struct Search;

  struct MovePicker {
  private:
    enum class Stage {
      generate_moves,
      emit_noisy,
      emit_quiet,
      end,
    };

    auto is_in_quiet_stage() const -> bool {
      return m_stage == Stage::emit_quiet;
    }

    Stage m_stage = Stage::generate_moves;

    const Search& m_search;
    const Position& m_position;

    usize m_current_index = 0;
    MoveList m_moves;
    std::span<Move> m_noisy;
    std::span<Move> m_quiet;

  public:
    MovePicker(const Search& search, const Position& position);

    auto next() -> Move;

  private:
    auto generate_moves() -> void;
  };

}  // namespace rose
