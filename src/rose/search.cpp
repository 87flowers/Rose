#include "rose/search.h"

#include <bit>
#include <cstdio>
#include <mutex>
#include <print>
#include <random>
#include <thread>

#include "rose/eval/eval.h"
#include "rose/eval/network.h"
#include "rose/game.h"
#include "rose/history.h"
#include "rose/line.h"
#include "rose/move_picker.h"
#include "rose/movegen.h"
#include "rose/search_control.h"
#include "rose/see.h"
#include "rose/tunable.h"
#include "rose/util/assert.h"
#include "rose/util/defer.h"
#include "rose/util/types.h"

namespace rose {

  namespace nodetype {
    struct Root;
    struct Pv;
    struct NonPv;

    struct Root {
      inline static constexpr bool is_root = true;
      inline static constexpr bool is_pv = true;
      inline static constexpr bool is_nmr = false;
      using Next = Pv;
    };

    struct Pv {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = true;
      inline static constexpr bool is_nmr = false;
      using Next = Pv;
    };

    struct NonPv {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = false;
      inline static constexpr bool is_nmr = false;
      using Next = NonPv;
    };

    struct Nmred {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = false;
      inline static constexpr bool is_nmr = true;
      using Next = Nmred;
    };
  } // namespace nodetype

  Search::Search(usize id, SearchShared &shared) : m_id(id), m_shared(shared) { reset(); }

  auto Search::reset() -> void {
    m_history.clear();
    setGame(Game{});
  }

  auto Search::setGame(const Game &g) -> void {
    m_root = g.position();
    m_move_stack = g.moveStack();
    m_hash_stack = g.hashStack();
    m_hash_waterline = std::max<usize>(1, m_hash_stack.size()) - 1;

    m_move_stack.reserve(m_move_stack.size() + max_search_ply + 1);
    m_hash_stack.reserve(m_hash_stack.size() + max_search_ply + 1);
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
        std::visit([this](const auto &ctrl) { this->searchRoot(ctrl); }, m_shared.ctrl);
      } else {
        searchRoot<controls::None>({});
      }
    }
  }

  template <typename Controls> auto Search::searchRoot(const Controls &ctrl) -> void {
    const auto print_info = [this, &ctrl](i32 depth, i32 score, const Line &pv) {
      const u64 nodes = m_shared.totalNodes();
      const time::Duration elapsed = ctrl.elapsed();
      m_shared.output->info({.depth = depth, .score = score, .time = elapsed, .nodes = nodes, .pv = pv});
    };

    Line last_pv;
    i32 last_score;
    i32 last_depth;

    for (i32 depth = 1; depth < max_search_ply; depth++) {
      Line pv{};
      const i32 score = search<nodetype::Root>(ctrl, m_root, pv, eval::min_score, eval::max_score, 0, depth);

      if (hasStopped())
        break;

      last_score = score;
      last_pv = pv;
      last_depth = depth;

      if (isMainThread()) {
        if (ctrl.checkSoftTermination(stats(), depth))
          break;
        print_info(depth, score, pv);
      }
    }

    requestStop();

    if (isMainThread()) {
      print_info(last_depth, last_score, last_pv);
      m_shared.output->bestmove(last_pv.pv.size() > 0 ? last_pv.pv[0] : Move::none());
    }
  }

  inline auto Search::isRuleDraw(const Position &position, i32 ply) -> std::optional<i32> {
    return position.isRuleDraw(m_hash_stack, m_hash_waterline, ply);
  }

  inline auto Search::ttLoad(const Position &position, int ply) const -> tt::LookupResult {
    return m_shared.transposition_table.load(position.hash(), ply);
  }
  inline auto Search::ttStore(const Position &position, int ply, tt::LookupResult lr) -> void {
    m_shared.transposition_table.store(position.hash(), ply, lr);
  }

  template <typename NodeT, typename Controls>
  auto Search::search(const Controls &ctrl, const Position &position, Line &pv, i32 alpha, i32 beta, i32 ply, i32 depth) -> i32 {

    if (depth <= 0)
      return quiesce<NodeT>(ctrl, position, pv, alpha, beta, ply);

    if (!NodeT::is_root && isMainThread() && ctrl.checkHardTermination(stats(), depth)) [[unlikely]] {
      requestStop();
      return 0;
    }
    stats().nodes.fetch_add(1, std::memory_order::relaxed);

    const bool is_in_check = position.isInCheck();

    if constexpr (!NodeT::is_root)
      if (const auto score = isRuleDraw(position, ply))
        return *score;
    if (ply >= max_search_ply) [[unlikely]]
      return is_in_check ? 0 : eval::evaluate(position);

    // Mate distance pruning
    if constexpr (!NodeT::is_root) {
      alpha = std::max(alpha, eval::mated(ply));
      beta = std::min(beta, eval::mating(ply + 1));
      if (alpha >= beta)
        return alpha;
    }

    const auto tte = ttLoad(position, ply);

    if constexpr (!NodeT::is_pv) {
      // Transposition table pruning
      if (tte.depth >= depth && [&] {
            switch (tte.bound) {
            case tt::Bound::none:
              return false;
            case tt::Bound::lower_bound:
              return tte.score >= beta;
            case tt::Bound::exact:
              return true;
            case tt::Bound::upper_bound:
              return tte.score <= alpha;
            }
          }()) {
        return tte.score;
      }

      if (!is_in_check) {
        const i32 static_eval = eval::evaluate(position);

        // Reverse futility pruning
        if (depth <= tunable::rfp_max_depth && static_eval - tunable::rfp_margin * depth >= beta)
          return static_eval;

        // Null move reductions
        if (depth >= tunable::nmr_min_depth && static_eval >= beta) {
          const i32 null_score = [&] {
            const Position child_position = position.moveNull();
            m_hash_stack.push_back(child_position.hash());
            m_move_stack.push_back(Move::none());
            rose_defer {
              m_hash_stack.pop_back();
              m_move_stack.pop_back();
            };
            const i32 reduction = tunable::nmr_zws_base;
            Line child_pv{};
            return -search<nodetype::NonPv>(ctrl, child_position, child_pv, -beta, -(beta - 1), ply, depth - reduction);
          }();

          if (null_score >= beta) {
            if constexpr (NodeT::is_nmr)
              return eval::isTheoretical(null_score) ? beta : null_score;

            depth -= tunable::nmr_reduction_base;
            return search<nodetype::Nmred>(ctrl, position, pv, alpha, beta, ply, depth);
          }
        }
      }
    }

    MovePicker moves{*this, position, tte.move};

    i32 best_score = eval::no_moves;
    Move best_move = tte.move;
    tt::Bound tt_bound = tt::Bound::upper_bound;
    usize moves_searched = 0;

    for (Move m = moves.next(); m != Move::none(); m = moves.next()) {
      const Position child_position = position.move(m);
      m_hash_stack.push_back(child_position.hash());
      m_move_stack.push_back(m);
      rose_defer {
        m_hash_stack.pop_back();
        m_move_stack.pop_back();
      };

      Line child_pv{};
      const i32 child_score = [&] {
        // Late move reductions
        if (moves_searched > tunable::lmr_move_threshold && depth > tunable::lmr_depth_threshold) {
          const int l2m = std::bit_width(moves_searched);
          const int l2d = std::bit_width(static_cast<u32>(depth)) + !NodeT::is_pv;
          i32 reduction = (tunable::lmr_base_const + l2m * l2d) / tunable::lmr_base_scale;
          reduction = std::clamp(reduction, 1, depth - 1);
          if (reduction > 1) {
            const i32 lmr_score = -search<nodetype::NonPv>(ctrl, child_position, child_pv, -(alpha + 1), -alpha, ply + 1, depth - reduction);
            if (lmr_score <= alpha)
              return lmr_score;
          }
        }

        // PVS scout search
        if (NodeT::is_pv && moves_searched > 0) {
          const i32 scout_score = -search<nodetype::NonPv>(ctrl, child_position, child_pv, -(alpha + 1), -alpha, ply + 1, depth - 1);
          if (scout_score <= alpha)
            return scout_score;
        }

        // PVS full-window search
        return -search<typename NodeT::Next>(ctrl, child_position, child_pv, -beta, -alpha, ply + 1, depth - 1);
      }();

      moves_searched++;

      if (hasStopped())
        return 0;

      if (child_score > best_score) {
        best_score = child_score;

        if (child_score > alpha) {
          alpha = child_score;
          best_move = m;
          tt_bound = tt::Bound::exact;
          moves.setMarker();

          if constexpr (NodeT::is_pv)
            pv.write(m, std::move(child_pv));

          if (child_score >= beta) {
            tt_bound = tt::Bound::lower_bound;
            break;
          }
        }
      }

      if (!NodeT::is_pv && !is_in_check && best_score >= eval::min_normal_score) {
        // Late move pruning
        if (moves_searched >= tunable::lmp_base + depth * depth / tunable::lmp_scale)
          moves.skipQuiets();
      }
    }

    if (moves_searched == 0)
      return is_in_check ? eval::mated(ply) : 0;

    // Update histories
    if (best_score >= beta && !best_move.capture()) {
      m_history.updateQuietHistory(+1, best_move, depth);

      const std::span<Move> bad_moves = moves.getMarkedQuiets();
      for (Move badm : bad_moves)
        m_history.updateQuietHistory(-1, badm, depth);
    }

    ttStore(position, ply,
            {
                .depth = depth,
                .bound = tt_bound,
                .score = best_score,
                .move = best_move,
            });

    return best_score;
  }

  template <typename NodeT, typename Controls>
  auto Search::quiesce(const Controls &ctrl, const Position &position, Line &pv, i32 alpha, i32 beta, i32 ply) -> i32 {
    const bool is_in_check = position.isInCheck();

    stats().nodes.fetch_add(1, std::memory_order::relaxed);

    if (const auto score = isRuleDraw(position, ply))
      return *score;
    if (ply >= max_search_ply) [[unlikely]]
      return is_in_check ? 0 : eval::evaluate(position);

    const i32 static_eval = is_in_check ? eval::no_moves : eval::evaluate(position);

    // Stand pat
    if (static_eval >= beta)
      return static_eval;
    alpha = std::max(alpha, static_eval);

    MovePicker moves{*this, position, Move::none()};
    if (!is_in_check)
      moves.skipQuiets();

    i32 best_score = static_eval;
    Move best_move = Move::none();
    tt::Bound tt_bound = tt::Bound::upper_bound;
    usize moves_searched = 0;

    for (Move m = moves.next(); m != Move::none(); m = moves.next()) {
      if (!eval::isLoss(best_score) && !see::see(position, m, tunable::qs_see_threshold)) {
        continue;
      }

      const Position child_position = position.move(m);
      m_hash_stack.push_back(child_position.hash());
      m_move_stack.push_back(m);
      rose_defer {
        m_hash_stack.pop_back();
        m_move_stack.pop_back();
      };

      Line child_pv{};
      const i32 child_score = -quiesce<typename NodeT::Next>(ctrl, child_position, child_pv, -beta, -alpha, ply + 1);

      moves_searched++;

      if (hasStopped())
        return 0;

      if (child_score > best_score) {
        best_score = child_score;

        if (child_score > alpha) {
          alpha = child_score;
          best_move = m;
          tt_bound = tt::Bound::exact;
          moves.setMarker();

          if constexpr (NodeT::is_pv)
            pv.write(m, std::move(child_pv));

          if (child_score >= beta) {
            tt_bound = tt::Bound::lower_bound;
            break;
          }
        }
      }
    }

    if (is_in_check && moves_searched == 0)
      return eval::mated(ply);

    return best_score;
  }

} // namespace rose
