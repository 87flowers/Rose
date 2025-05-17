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
      emit_noisy,
      emit_quiet,
      end,
    };

    Stage m_stage = Stage::tt_move;

    const Search &m_search;
    Move m_tt_move;

    usize m_current_index = 0;
    MoveList m_moves;
    std::span<Move> m_noisy;
    std::span<Move> m_quiet;

  public:
    MovePicker(const Search &search, Move tt_move);

    auto next() -> Move;

  private:
    auto generateMoves() -> void;
    auto sortNoisy() -> void;
    auto sortQuiet() -> void;
  };

} // namespace rose
