#include "rose/move_picker.h"

#include <ranges>
#include <utility>

#include "rose/search.h"

namespace rose {

  MovePicker::MovePicker(const Search &search, Move tt_move) : m_search(search), m_tt_move(tt_move) {}

#define EMIT(list)                                                                                                                                   \
  if (m_current_index < list.size() && list[m_current_index] == m_tt_move)                                                                           \
    m_current_index++;                                                                                                                               \
  if (m_current_index < list.size())                                                                                                                 \
    return list[m_current_index++];                                                                                                                  \
  m_current_index = 0;

  auto MovePicker::next() -> Move {
    switch (m_stage) {
    case Stage::tt_move:
      if (m_tt_move != Move::none()) {
        m_stage = Stage::generate_moves;
        return m_tt_move;
      }
      [[fallthrough]];
    case Stage::generate_moves:
      m_stage = Stage::generate_moves;
      {
        MoveGen movegen(m_search.m_game.position(), m_search.m_shared.movegen_precomp);
        movegen.generateMoves({m_unprotected_noisy, m_protected_noisy, m_unprotected_quiet, m_protected_quiet});
      }
      [[fallthrough]];
    case Stage::sort_unprotected_noisy:
      m_stage = Stage::sort_unprotected_noisy;
      mvvlva(m_unprotected_noisy);
      [[fallthrough]];
    case Stage::emit_unprotected_noisy:
      m_stage = Stage::emit_unprotected_noisy;
      EMIT(m_unprotected_noisy);
      [[fallthrough]];
    case Stage::sort_protected_noisy:
      m_stage = Stage::sort_protected_noisy;
      mvvlva(m_protected_noisy);
      [[fallthrough]];
    case Stage::emit_protected_noisy:
      m_stage = Stage::emit_protected_noisy;
      EMIT(m_protected_noisy);
      [[fallthrough]];
    case Stage::sort_unprotected_quiet:
      m_stage = Stage::sort_unprotected_quiet;
      [[fallthrough]];
    case Stage::emit_unprotected_quiet:
      m_stage = Stage::emit_unprotected_quiet;
      EMIT(m_unprotected_quiet);
      [[fallthrough]];
    case Stage::sort_protected_quiet:
      m_stage = Stage::sort_protected_quiet;
      [[fallthrough]];
    case Stage::emit_protected_quiet:
      m_stage = Stage::emit_protected_quiet;
      EMIT(m_protected_quiet);
      [[fallthrough]];
    case Stage::end:
    default:
      m_stage = Stage::end;
      return Move::none();
    }
  }

#undef EMIT

  auto MovePicker::mvvlva(MoveList &noisies) -> void {
    const std::array<Place, 64> &board = m_search.m_game.position().board().m;

    StaticVector<i32, max_legal_moves> noisy_scores;
    noisy_scores.resize(noisies.size());
    for (isize i = 0; i < noisies.size(); i++) {
      const Move m = noisies[i];
      const PieceType src_ptype = board[m.from().raw].ptype();
      const PieceType dst_ptype = board[m.to().raw].ptype();
      noisy_scores[i] = (dst_ptype.toSortValue() * 16 - src_ptype.toSortValue()) * 1024 - i;
    }

    std::ranges::sort(std::ranges::zip_view(noisies, noisy_scores), [](auto &&a, auto &&b) { return std::get<1>(a) > std::get<1>(b); });
  }

} // namespace rose
