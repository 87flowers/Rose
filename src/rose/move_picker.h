#pragma once

#include <optional>
#include <span>

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
      emit_good_noisy,
      emit_quiet,
      emit_bad_noisy,
      end,
    };

    auto is_in_quiet_stage() const -> bool { return m_stage > Stage::emit_good_noisy && m_stage < Stage::emit_bad_noisy; }

    Stage m_stage = Stage::tt_move;

    const Search &m_search;
    const Position &m_position;
    Move m_tt_move;
    Move m_killer;

    usize m_current_index = 0;
    MoveList m_moves;
    std::span<Move> m_noisy;
    std::span<Move> m_quiet;
    MoveList m_bad_noisy;

    bool m_skip_quiets = false;
    usize m_quiet_marker = 0;

  public:
    MovePicker(const Search &search, const Position &position, Move tt_move, Move killer);

    auto skipQuiets() -> void;

    auto next() -> Move;

    auto setMarker() -> void {
      if (m_stage == Stage::emit_quiet)
        m_quiet_marker = m_current_index;
    }
    auto getMarkedQuiets() const -> std::span<Move> { return m_quiet.subspan(0, std::max<usize>(1, m_quiet_marker) - 1); }

  private:
    auto generateMoves() -> void;
    auto sortNoisy() -> void;
    auto sortQuiet() -> void;
  };

} // namespace rose
