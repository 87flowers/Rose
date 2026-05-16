#include "rose/move_picker.hpp"

#include "rose/movegen.hpp"
#include "rose/search.hpp"
#include "rose/see.hpp"
#include "rose/util/static_vector.hpp"

#include <algorithm>
#include <ranges>

namespace rose {

  MovePicker::MovePicker(const Search& search, const Position& position, const SearchStack* ss, Move tt_move) :
      m_search(search),
      m_position(position),
      m_ss(ss),
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

      m_stage = Stage::emit_good_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_good_noisy:
      while (m_current_index < m_moves.size()) {
        const Move mv = m_moves[m_current_index++];
        if (!see::see(m_position, mv, -150)) {
          m_bad_noisies.push_back(mv);
          continue;
        }
        if (mv == m_tt_move)
          continue;
        return mv;
      }

      m_stage = Stage::generate_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::generate_quiet:
      if (m_skip_quiet) {
        m_stage = Stage::emit_bad_noisy;
        m_current_index = 0;
        return next();
      }

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

      m_stage = Stage::emit_bad_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_bad_noisy:
      while (m_current_index < m_bad_noisies.size()) {
        const Move mv = m_bad_noisies[m_current_index++];
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

    StaticVector<i32, max_legal_moves> scores;
    scores.resize(m_moves.size());

    const Color stm = m_position.stm();

    constexpr std::array<i32, 8> victim_score {{0, 100000, 1000, 3000, 0, 3000, 5000, 9000}};

    for (isize i = 0; i < m_moves.size(); i++) {
      const Move mv = m_moves[i];
      const PieceType victim = m_position.ptype_at(mv.to());

      i32 score;
      score += victim_score[victim.to_index()];
      score += m_search.m_noisy_history.get(stm, mv, victim);

      scores[i] = score * 256 - i;
    }

    std::ranges::sort(std::ranges::zip_view(m_moves, scores), [](auto&& a, auto&& b) {
      return std::get<1>(a) > std::get<1>(b);
    });
  }

  auto MovePicker::generate_quiet() -> void {
    m_moves.clear();
    m_movegen.generate_quiet(m_moves);

    StaticVector<i32, max_legal_moves> scores;
    scores.resize(m_moves.size());

    const Color stm = m_position.stm();

    for (isize i = 0; i < m_moves.size(); i++) {
      const Move mv = m_moves[i];
      const PieceType ptype = m_position.place_at(mv.from()).ptype();

      i32 score = 0;
      score += m_search.m_quiet_history.get(stm, mv);
      for (i32 i : {1, 2})
        if (m_ss[-i].conthist)
          score += m_ss[-i].conthist->get(stm, ptype, mv);

      scores[i] = score * 256 - i;
    }

    std::ranges::sort(std::ranges::zip_view(m_moves, scores), [](auto&& a, auto&& b) {
      return std::get<1>(a) > std::get<1>(b);
    });
  }

}  // namespace rose
