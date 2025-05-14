#include "rose/search.h"

#include <cstdio>
#include <mutex>
#include <print>
#include <random>
#include <thread>

#include "rose/eval/eval.h"
#include "rose/eval/hce.h"
#include "rose/game.h"
#include "rose/line.h"
#include "rose/movegen.h"
#include "rose/search_control.h"
#include "rose/util/assert.h"
#include "rose/util/defer.h"
#include "rose/util/types.h"

namespace rose {

  Search::Search(usize id, SearchShared &shared) : m_id(id), m_shared(shared) { reset(); }

  auto Search::reset() -> void { m_game.reset(); }

  auto Search::setGame(const Game &g) -> void {
    m_game = g;
    m_game.setHashWaterline();
  }

  auto Search::requestStart() -> void {
    rose_assert(!m_thread.joinable());
    m_thread = std::jthread([this](std::stop_token quit) { this->threadMain(quit); });
  }

  auto Search::threadMain(std::stop_token quit) -> void {
    while (true) {
      m_shared.idle_barrier.arrive_and_wait();

      if (quit.stop_requested()) [[unlikely]]
        break;

      std::shared_lock _{m_shared.mutex};
      (void)m_shared.started_barrier.arrive();

      stats().reset();

      if (isMainThread()) {
        std::visit([this](const auto &ctrl) { this->go(ctrl); }, m_shared.ctrl);
      } else {
        go<controls::None>({});
      }
    }
  }

  template <typename Controls> auto Search::go(const Controls &ctrl) -> void {
    const auto print_info = [this, &ctrl](i32 depth, i32 score, const Line &pv) {
      if (!isMainThread())
        return;

      const u64 nodes = m_shared.totalNodes();
      const time::Duration elapsed = ctrl.elapsed();
      const time::Milliseconds elapsed_ms = time::cast<time::Milliseconds>(elapsed);
      const u64 nps = time::nps<u64>(nodes, elapsed);
      std::print("info depth {} score cp {} time {} nodes {} nps {} pv {}\n", depth, score, elapsed_ms.count(), nodes, nps, pv);
    };

    Line last_pv;
    i32 last_score;
    i32 last_depth;

    for (i32 depth = 1; depth < max_search_ply; depth++) {
      Line pv{};
      const i32 score = searchRoot(ctrl, pv, depth);

      if (hasStopped())
        break;

      last_score = score;
      last_pv = pv;
      last_depth = depth;

      if (isMainThread() && ctrl.checkSoftTermination(stats(), depth))
        break;

      print_info(depth, score, pv);
    }

    requestStop();

    print_info(last_depth, last_score, last_pv);
    std::print("bestmove {}\n", last_pv.pv[0]);
    std::fflush(stdout);
  }

  inline auto Search::isDraw(bool is_in_check, i32 ply) -> std::optional<i32> {
    if (m_game.isRepetition())
      return 0;
    if (m_game.position().fiftyMoveClock() >= 100) {
      if (!is_in_check)
        return 0;
      [[unlikely]];
      MoveList moves;
      MoveGen movegen{m_game.position(), m_shared.movegen_precomp};
      movegen.generateMoves(moves);
      return moves.size() == 0 ? eval::mated(ply) : 0;
    }
    return std::nullopt;
  }

  static auto nextGuess(i32 alpha, i32 beta, usize subtree_count) -> i32 { return alpha + (beta - alpha) * (subtree_count - 1) / subtree_count; }

  template <typename Controls> auto Search::searchRoot(const Controls &ctrl, Line &pv, i32 depth) -> i32 {
    const i32 ply = 1;
    const bool is_in_check = m_game.position().isInCheck();
    if (const auto score = isDraw(is_in_check, ply))
      return *score;

    MoveList moves;
    {
      MoveGen movegen{m_game.position(), m_shared.movegen_precomp};
      movegen.generateMoves(moves);
    }

    if (moves.size() == 0)
      return is_in_check ? eval::mated(ply) : 0;

    i32 alpha = eval::min_score;
    i32 beta = eval::max_score;
    usize subtree_count = moves.size();
    usize better_count;
    i32 test;

    do {
      better_count = 0;
      test = nextGuess(alpha, beta, moves.size());

      MoveList new_candidates;

      for (const auto m : moves) {
        m_game.move(m);
        rose_defer { m_game.unmove(); };

        Line child_pv{};
        const i32 child_score = -search(ctrl, child_pv, -test, -(test - 1), ply + 1, depth - 1);
        if (child_score >= test) {
          pv.write(m, std::move(child_pv));
          new_candidates.push_back(m);
        }
      }

      if (hasStopped())
        return 0;

      if (new_candidates.size() > 0) {
        alpha = test;
        moves = new_candidates;
      } else {
        beta = test;
      }

    } while (beta - alpha > 1 && moves.size() > 1);

    return test;
  }

  template <typename Controls> auto Search::search(const Controls &ctrl, Line &pv, i32 alpha, i32 beta, i32 ply, i32 depth) -> i32 {
    const bool is_in_check = m_game.position().isInCheck();

    if (isMainThread() && ply > 1 && ctrl.checkHardTermination(stats(), depth)) [[unlikely]] {
      requestStop();
      return 0;
    }
    stats().nodes.fetch_add(1, std::memory_order::relaxed);

    if (const auto score = isDraw(is_in_check, ply))
      return *score;
    if (depth <= 0)
      return eval::hce(m_game.position());
    if (ply >= max_search_ply) [[unlikely]]
      return is_in_check ? 0 : eval::hce(m_game.position());

    MoveList moves;
    {
      MoveGen movegen{m_game.position(), m_shared.movegen_precomp};
      movegen.generateMoves(moves);
    }

    if (moves.size() == 0)
      return is_in_check ? eval::mated(ply) : 0;

    i32 best_score = eval::no_moves;
    for (const auto m : moves) {
      m_game.move(m);
      rose_defer { m_game.unmove(); };

      Line child_pv{};
      const i32 child_score = -search(ctrl, child_pv, -beta, -alpha, ply + 1, depth - 1);

      if (hasStopped())
        return 0;

      if (child_score > best_score) {
        best_score = child_score;
        if (child_score > alpha) {
          alpha = child_score;
          pv.write(m, std::move(child_pv));
          if (child_score >= beta)
            break;
        }
      }
    }

    return best_score;
  }

} // namespace rose
