#include "rose/move_picker.h"

#include <utility>

#include "rose/search.h"

namespace rose {

  MovePicker::MovePicker(const Search &search, Move tt_move)
      : m_search(search), m_tt_move(tt_move), m_movegen(search.m_game.position(), search.m_shared.movegen_precomp) {}

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::tt_move:
      m_stage = Stage::generate_moves;
      if (m_tt_move != Move::none()) {
        return m_tt_move;
      }
      [[fallthrough]];
    case Stage::generate_moves:
      m_moves.clear();
      m_movegen.generateMoves(m_moves);
      m_stage = Stage::emit_moves;
      [[fallthrough]];
    case Stage::emit_moves:
      if (m_current_index < m_moves.size() && m_moves[m_current_index] == m_tt_move)
        m_current_index++;
      if (m_current_index < m_moves.size())
        return m_moves[m_current_index++];
      [[fallthrough]];
    default:
      return Move::none();
    }
  }

} // namespace rose
