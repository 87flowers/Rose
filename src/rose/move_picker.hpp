#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/movegen.hpp"

namespace rose {

  struct Search;

  struct MovePicker {
  private:
    enum class Stage {
      tt_move,
      generate_noisy,
      emit_noisy,
      generate_quiet,
      emit_quiet,
      end,
    };

    auto is_in_quiet_stage() const -> bool {
      return m_stage == Stage::emit_quiet;
    }

    Stage m_stage = Stage::generate_noisy;

    const Search& m_search;
    const Position& m_position;
    MoveGen m_movegen;
    Move m_tt_move;

    usize m_current_index = 0;
    MoveList m_moves;

  public:
    MovePicker(const Search& search, const Position& position, Move tt_move);

    auto next() -> Move;

  private:
    auto generate_noisy() -> void;
    auto generate_quiet() -> void;
  };

}  // namespace rose
