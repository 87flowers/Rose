#include "rose/move_picker.hpp"

#include "rose/bitboard.hpp"
#include "rose/movegen.hpp"
#include "rose/search.hpp"
#include "rose/see.hpp"
#include "rose/tune.hpp"
#include "rose/util/static_vector.hpp"

#include <algorithm>
#include <ranges>

namespace rose {

  MovePicker::MovePicker(const SearchData& sd, const Position& position, const SearchStack* ss, Move hint_move) :
      m_sd(sd),
      m_position(position),
      m_ss(ss),
      m_movegen(position),
      m_hint_move(hint_move) {
  }

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::hint_move:
      m_stage = Stage::generate_noisy;

      if (m_hint_move.is_some()) {
        return m_hint_move;
      }

      [[fallthrough]];
    case Stage::generate_noisy:
      generate_noisy();

      m_stage = Stage::emit_good_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_good_noisy:
      while (m_current_index < m_moves.size()) {
        const Move mv = m_moves[m_current_index++];
        if (!see::see(m_position, mv, -163_z)) {
          m_bad_noisies.push_back(mv);
          continue;
        }
        if (mv == m_hint_move)
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
        if (mv == m_hint_move)
          continue;
        return mv;
      }

      m_stage = Stage::emit_bad_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_bad_noisy:
      while (m_current_index < m_bad_noisies.size()) {
        const Move mv = m_bad_noisies[m_current_index++];
        if (mv == m_hint_move)
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

    const std::array<i32, 8> victim_score {{0, 9480_z, 103_z, 280_z, 0, 355_z, 474_z, 1009_z}};

    for (isize i = 0; i < m_moves.size(); i++) {
      const Move mv = m_moves[i];
      const PieceType victim = m_position.ptype_at(mv.to());
      const PieceType attacker = m_position.ptype_at(mv.from());

      i32 score = 0;
      score += victim_score[victim.to_index()] * 7_z;
      score += m_sd.noisy_history.get(stm, attacker, mv);

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
    const Bitboard threats = m_position.attack_table(!stm).bitboard_any();

    for (isize i = 0; i < m_moves.size(); i++) {
      const Move mv = m_moves[i];
      const PieceType ptype = m_position.ptype_at(mv.from());

      i32 score = 0;
      score += m_sd.quiet_history.get(stm, threats, mv);
      for (i32 i : conthists_indexes)
        if (m_ss[-i].conthist)
          score += m_ss[-i].conthist->get(stm, ptype, mv);

      scores[i] = score * 256 - i;
    }

    std::ranges::sort(std::ranges::zip_view(m_moves, scores), [](auto&& a, auto&& b) {
      return std::get<1>(a) > std::get<1>(b);
    });
  }

}  // namespace rose
