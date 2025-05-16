#include "rose/move_picker.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>

#include "rose/common.h"
#include "rose/search.h"
#include "rose/util/static_vector.h"

namespace rose {

  MovePicker::MovePicker(const Search &search, Move tt_move) : m_search(search), m_tt_move(tt_move) {}

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
      {
        MoveGen movegen(m_search.m_game.position(), m_search.m_shared.movegen_precomp);
        movegen.generateMoves(m_moves);
        sortMoves();
      }
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

  auto MovePicker::sortMoves() -> void {
    const std::array<Place, 64> &board = m_search.m_game.position().board().m;

    m_first_quiet = std::stable_partition(m_moves.begin(), m_moves.end(), [](Move m) { return m.capture(); });
    const std::span noisy{m_moves.begin(), m_first_quiet};
    const isize noisy_count = noisy.size();

    StaticVector<i32, max_legal_moves> noisy_scores;
    noisy_scores.resize(noisy_count);
    for (isize i = 0; i < noisy_count; i++) {
      const Move m = m_moves[i];
      const PieceType src_ptype = board[m.from().raw].ptype();
      const PieceType dst_ptype = board[m.to().raw].ptype();
      noisy_scores[i] = (dst_ptype.toSortValue() * 16 - src_ptype.toSortValue()) * 1024 - i;
    }

    std::ranges::sort(std::ranges::zip_view(noisy, noisy_scores), [](auto &&a, auto &&b) { return std::get<1>(a) > std::get<1>(b); });
  }

} // namespace rose
