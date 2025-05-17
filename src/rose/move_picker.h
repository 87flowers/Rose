#pragma once

#include "rose/move.h"
#include "rose/movegen.h"
#include "rose/util/types.h"

namespace rose {

  struct Search;

  struct MovePicker {
  private:
    enum class Stage {
      tt_move,
      generate_moves,
      sort_unprotected_noisy,
      emit_unprotected_noisy,
      sort_protected_noisy,
      emit_protected_noisy,
      sort_unprotected_quiet,
      emit_unprotected_quiet,
      sort_protected_quiet,
      emit_protected_quiet,
      end,
    };

    Stage m_stage = Stage::tt_move;

    const Search &m_search;
    Move m_tt_move;

    usize m_current_index = 0;
    MoveList m_unprotected_noisy;
    MoveList m_protected_noisy;
    MoveList m_unprotected_quiet;
    MoveList m_protected_quiet;

  public:
    MovePicker(const Search &search, Move tt_move);

    auto next() -> Move;

  private:
    auto mvvlva(MoveList &noisies) -> void;
  };

} // namespace rose
