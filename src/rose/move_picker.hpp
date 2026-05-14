#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/movegen.hpp"
#include "rose/search.hpp"

namespace rose {

  struct Search;

  struct MovePicker {
  private:
    enum class Stage {
      tt_move,
      generate_noisy,
      emit_good_noisy,
      generate_quiet,
      emit_quiet,
      emit_bad_noisy,
      end,
    };

    auto is_in_quiet_stage() const -> bool {
      return m_stage >= Stage::generate_quiet && m_stage <= Stage::emit_quiet;
    }

    Stage m_stage = Stage::tt_move;

    const Search& m_search;
    const Position& m_position;
    const SearchStack* m_ss;
    MoveGen m_movegen;
    Move m_tt_move;

    bool m_skip_quiet = false;
    usize m_current_index = 0;
    MoveList m_moves;
    MoveList m_bad_noisies;

  public:
    MovePicker(const Search& search, const Position& position, const SearchStack* ss, Move tt_move);

    auto next() -> Move;

    auto skip_quiet() -> void {
      m_skip_quiet = true;
      if (is_in_quiet_stage()) {
        m_stage = Stage::emit_bad_noisy;
        m_current_index = 0;
      }
    }

  private:
    auto generate_noisy() -> void;
    auto generate_quiet() -> void;
  };

}  // namespace rose
