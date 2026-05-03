#include "rose/move_picker.hpp"

#include "rose/movegen.hpp"
#include "rose/search.hpp"

namespace rose {

  MovePicker::MovePicker(const Search& search, const Position& position) :
      m_search(search),
      m_position(position),
      m_movegen(position) {
  }

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::generate_noisy:
      generate_noisy();

      m_stage = Stage::emit_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_noisy:
      if (m_current_index < m_moves.size())
        return m_moves[m_current_index++];

      m_stage = Stage::generate_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::generate_quiet:
      generate_quiet();

      m_stage = Stage::emit_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_quiet:
      if (m_current_index < m_moves.size())
        return m_moves[m_current_index++];

      m_stage = Stage::end;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::end:
    default:
      return Move::none();
    }
  }

  auto MovePicker::generate_noisy() -> void {
    m_moves.clear();
    m_movegen.prepare();
    m_movegen.generate_noisy(m_moves);
  }

  auto MovePicker::generate_quiet() -> void {
    m_moves.clear();
    m_movegen.generate_quiet(m_moves);
  }

}  // namespace rose
