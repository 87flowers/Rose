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

    enum class SkipMode {
      none,
      quiet,
      quiet_and_bad_noisy,
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

    SkipMode m_skip = SkipMode::none;
    usize m_current_index = 0;
    MoveList m_moves;
    MoveList m_bad_noisies;

  public:
    MovePicker(const Search& search, const Position& position, const SearchStack* ss, Move tt_move);

    auto next() -> Move;

    auto skip_quiet() -> void {
      rose_assert(m_skip == SkipMode::none);
      m_skip = SkipMode::quiet;
      if (is_in_quiet_stage()) {
        m_stage = Stage::emit_bad_noisy;
        m_current_index = 0;
      }
    }

    auto skip_quiet_and_bad_noisy() -> void {
      rose_assert(m_skip == SkipMode::none);
      m_skip = SkipMode::quiet_and_bad_noisy;
      if (m_stage >= Stage::generate_quiet) {
        m_stage = Stage::end;
        m_current_index = 0;
      }
    }

  private:
    auto generate_noisy() -> void;
    auto generate_quiet() -> void;
  };

}  // namespace rose
