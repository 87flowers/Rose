#include "rose/move_picker.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>

#include "rose/common.h"
#include "rose/search.h"
#include "rose/util/static_vector.h"

namespace rose {

  MovePicker::MovePicker(const Search &search, const Position &position, Move tt_move) : m_search(search), m_position(position), m_tt_move(tt_move) {}

  auto MovePicker::skipQuiets() -> void {
    if (m_stage == Stage::emit_quiet)
      m_stage = Stage::end;
    m_skip_quiets = true;
  }

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::tt_move:
      m_stage = Stage::generate_moves;
      if (m_tt_move != Move::none()) {
        return m_tt_move;
      }
      [[fallthrough]];
    case Stage::generate_moves:
      generateMoves();

      sortNoisy();
      m_stage = Stage::emit_noisy;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_noisy:
      if (m_current_index < m_noisy.size() && m_noisy[m_current_index] == m_tt_move)
        m_current_index++;
      if (m_current_index < m_noisy.size())
        return m_noisy[m_current_index++];

      if (m_skip_quiets) {
        m_stage = Stage::end;
        return Move::none();
      }

      sortQuiet();
      m_stage = Stage::emit_quiet;
      m_current_index = 0;

      [[fallthrough]];
    case Stage::emit_quiet:
      if (m_current_index < m_quiet.size() && m_quiet[m_current_index] == m_tt_move)
        m_current_index++;
      if (m_current_index < m_quiet.size())
        return m_quiet[m_current_index++];

      m_stage = Stage::end;

      [[fallthrough]];
    case Stage::end:
    default:
      return Move::none();
    }
  }

  auto MovePicker::generateMoves() -> void {
    m_moves.clear();

    MoveGen movegen(m_position, m_search.m_shared.movegen_precomp);
    movegen.generateMoves(m_moves);

    const auto first_quiet = std::stable_partition(m_moves.begin(), m_moves.end(), [](Move m) { return m.capture(); });
    m_noisy = std::span{m_moves.begin(), first_quiet};
    m_quiet = std::span{first_quiet, m_moves.end()};
  }

  auto MovePicker::sortNoisy() -> void {
    const Byteboard &board = m_position.board();

    StaticVector<i32, max_legal_moves> noisy_scores;
    noisy_scores.resize(m_noisy.size());

    for (isize i = 0; i < m_noisy.size(); i++) {
      const Move m = m_noisy[i];
      const PieceType src_ptype = board.read(m.from()).ptype();
      const PieceType dst_ptype = board.read(m.to()).ptype();
      noisy_scores[i] = (dst_ptype.toSortValue() * 16 - src_ptype.toSortValue()) * 1024 - i;
    }

    std::ranges::sort(std::ranges::zip_view(m_noisy, noisy_scores), [](auto &&a, auto &&b) { return std::get<1>(a) > std::get<1>(b); });
  }

  auto MovePicker::sortQuiet() -> void {
    StaticVector<i32, max_legal_moves> quiet_scores;
    quiet_scores.resize(m_quiet.size());

    for (isize i = 0; i < m_quiet.size(); i++) {
      const Move m = m_quiet[i];
      quiet_scores[i] = m_search.m_history.getHistory(m) * 1024 - i;
    }

    std::ranges::sort(std::ranges::zip_view(m_quiet, quiet_scores), [](auto &&a, auto &&b) { return std::get<1>(a) > std::get<1>(b); });
  }

} // namespace rose
