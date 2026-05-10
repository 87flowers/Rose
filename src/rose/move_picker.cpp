#include "rose/move_picker.hpp"

#include "rose/movegen.hpp"
#include "rose/search.hpp"
#include "rose/util/static_vector.hpp"

#include <ranges>

namespace rose {

  MovePicker::MovePicker(const Search& search, const Position& position, Move tt_move) :
      m_search(search),
      m_position(position),
      m_movegen(position),
      m_tt_move(tt_move) {
  }

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::tt_move:
      m_stage = Stage::generate_noisy;

      if (m_position.is_legal_slow(m_tt_move)) {
        return m_tt_move;
      }

    case Stage::generate_noisy:
      generate_noisy();

      m_stage = Stage::emit_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_noisy:
      while (m_current_index < m_moves.size()) {
        const Move mv = m_moves[m_current_index++];
        if (mv == m_tt_move)
          continue;
        return mv;
      }

      m_stage = Stage::generate_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::generate_quiet:
      generate_quiet();

      m_stage = Stage::emit_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_quiet:
      while (m_current_index < m_moves.size()) {
        const Move mv = m_moves[m_current_index++];
        if (mv == m_tt_move)
          continue;
        return mv;
      }

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
    m_movegen.generate_noisy(m_moves);
  }

  auto MovePicker::generate_quiet() -> void {
    m_moves.clear();
    m_movegen.generate_quiet(m_moves);

    StaticVector<i32, max_legal_moves> scores;
    scores.resize(m_moves.size());

    const Color stm = m_position.stm();

    for (isize i = 0; i < m_moves.size(); i++) {
      const Move mv = m_moves[i];
      scores[i] = m_search.m_quiet_history.get(stm, mv) * 256 - i;
    }

    std::ranges::sort(std::ranges::zip_view(m_moves, scores), [](auto&& a, auto&& b) {
      return std::get<1>(a) > std::get<1>(b);
    });
  }

}  // namespace rose
