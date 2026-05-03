#include "rose/move_picker.hpp"

#include "rose/common.hpp"
#include "rose/search.hpp"
#include "rose/util/static_vector.hpp"

#include <algorithm>
#include <ranges>

namespace rose {

  MovePicker::MovePicker(const Search& search, const Position& position) :
      m_search(search),
      m_position(position) {
  }

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::generate_moves:
      generate_moves();

      m_stage = Stage::emit_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_noisy:
      if (m_current_index < m_noisy.size())
        return m_noisy[m_current_index++];

      m_stage = Stage::emit_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_quiet:
      if (m_current_index < m_quiet.size())
        return m_quiet[m_current_index++];

      m_stage = Stage::end;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::end:
    default:
      return Move::none();
    }
  }

  auto MovePicker::generate_moves() -> void {
    m_moves.clear();

    MoveGen movegen {m_position};
    movegen.generate_moves(m_moves);

    const auto first_quiet = std::stable_partition(m_moves.begin(), m_moves.end(), [](Move m) {
      return m.capture();
    });
    m_noisy = std::span {m_moves.begin(), first_quiet};
    m_quiet = std::span {first_quiet, m_moves.end()};
  }

}  // namespace rose
