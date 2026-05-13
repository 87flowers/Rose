#include "rose/search.hpp"

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/move_picker.hpp"
#include "rose/movegen.hpp"
#include "rose/nnue/nnue.hpp"
#include "rose/score.hpp"
#include "rose/search_control.hpp"
#include "rose/tt.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/defer.hpp"
#include "rose/util/time.hpp"

#include <atomic>
#include <bit>
#include <fmt/format.h>
#include <memory>
#include <thread>
#include <variant>

namespace rose {

  constexpr i32 max_depth = 250;

  namespace node {
    struct Root;
    struct Pv;
    struct NonPv;

    struct Root {
      inline static constexpr bool is_root = true;
      inline static constexpr bool is_pv = true;
      using next = Pv;
    };

    struct Pv {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = true;
      using next = Pv;
    };

    struct NonPv {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = false;
      using next = NonPv;
    };
  }  // namespace node

  auto SearchShared::reset() -> void {
    stats.clear();
    transposition_table.clear();
  }

  auto SearchShared::set_hash_size(int mb) -> void {
    rose_assert(mb > 0);
    transposition_table.resize(static_cast<usize>(mb));
  }

  auto SearchShared::set_output(std::shared_ptr<EngineOutput> output) -> void {
    this->output = output;
  }

  auto SearchShared::send_ping() -> void {
    engine_message = EngineMessage::ping;
    idle_barrier.arrive_and_wait();
    started_barrier.arrive_and_wait();
  }

  auto SearchShared::send_quit() -> void {
    engine_message = EngineMessage::quit;
    idle_barrier.arrive_and_wait();
  }

  auto SearchShared::send_go(time::TimePoint start_time, const SearchLimit& limits, const Game& g) -> void {
    search_start_time = start_time;
    search_main_limits = limits;
    search_game = &g;
    engine_message = EngineMessage::go;
    idle_barrier.arrive_and_wait();
    started_barrier.arrive_and_wait();
    search_game = nullptr;
  }

  auto SearchShared::stop() -> void {
    stopping = true;
  }

  auto Search::reset() -> void {
    m_quiet_history.reset();
  }

  auto Search::launch() -> void {
    rose_assert(!m_thread.joinable());
    m_thread = std::jthread([this] {
      this->thread_main();
    });
  }

  auto calc_time(const SearchLimit& limits, Color stm) -> std::tuple<time::Duration, time::Duration> {
    constexpr time::Milliseconds margin_ms {100};
    constexpr time::Milliseconds zero_ms {0};

    const auto remaining_time = stm == Color::white ? limits.wtime : limits.btime;
    const auto increment = stm == Color::white ? limits.winc : limits.binc;

    const time::Milliseconds remaining_time_ms {remaining_time.value_or(0)};
    const time::Milliseconds increment_ms {increment.value_or(0)};
    const int movestogo = limits.movestogo.value_or(20);

    time::Milliseconds safe_remaining_ms = std::max(remaining_time_ms - margin_ms, zero_ms);

    if (limits.movetime) [[unlikely]] {
      const time::Milliseconds movetime_ms {*limits.movetime};
      if (!remaining_time && !increment)
        return {movetime_ms, movetime_ms};
      safe_remaining_ms = std::min(safe_remaining_ms, movetime_ms);
    }

    const time::Milliseconds hard_limit = std::min(safe_remaining_ms / movestogo * 7 + increment_ms / 3, safe_remaining_ms);
    const time::Milliseconds soft_limit = std::min(safe_remaining_ms / movestogo + increment_ms / 3, safe_remaining_ms);

    return {hard_limit, soft_limit};
  }

  auto calc_ctrl(time::TimePoint start_time, const SearchLimit& limits, Color stm) -> controls::Any {
    if (limits.has_time && !limits.has_other) {
      controls::Time ctrl;

      ctrl.start_time = start_time;
      std::tie(ctrl.hard_time, ctrl.soft_time) = calc_time(limits, stm);

      return ctrl;
    } else if (limits.has_time || limits.has_other) {
      controls::All ctrl;

      ctrl.start_time = start_time;
      if (limits.has_time)
        std::tie(ctrl.hard_time, ctrl.soft_time) = calc_time(limits, stm);
      ctrl.hard_nodes = limits.hard_nodes;
      ctrl.soft_nodes = limits.soft_nodes;
      ctrl.depth = limits.depth;

      return ctrl;
    } else {
      return controls::None {start_time};
    }
  }

  auto Search::thread_main() -> void {
    while (true) {
      m_shared.idle_barrier.arrive_and_wait();

      switch (m_shared.engine_message) {
      case EngineMessage::ping:
        m_shared.started_barrier.arrive_and_wait();
        break;

      case EngineMessage::quit:
        return;

      case EngineMessage::go: {
        const Game& g = *m_shared.search_game;

        m_root = g.position();
        m_move_stack = g.move_stack();
        m_hash_stack = g.hash_stack();
        m_hash_waterline = std::max<usize>(1, m_hash_stack.size()) - 1;

        const auto ctrl = is_main_thread() ? calc_ctrl(m_shared.search_start_time, m_shared.search_main_limits, m_root.stm()) :
                                             controls::None {.start_time = m_shared.search_start_time};

        m_shared.stopping = false;
        stats().reset();

        (void)m_shared.started_barrier.arrive();

        std::visit(
          [this](const auto& ctrl) {
            this->search_root(ctrl);
          },
          ctrl);
      } break;
      }
    }
  }

  template<typename Controls>
  auto Search::search_root(const Controls& ctrl) -> void {
    Line last_pv;
    Score last_score = score::none;
    i32 last_depth = -1;

    const auto print_info = [&, this]() {
      m_shared.output->info(EngineOutput::Info {
        .depth = last_depth,
        .score = last_score,
        .time = ctrl.elapsed(),
        .nodes = m_shared.total_nodes(),
        .pv = last_pv,
      });
    };

    for (i32 depth = 1; depth < max_depth; depth++) {
      Line pv {};
      const Score score = search<node::Root>(ctrl, m_root, pv, -score::infinity, score::infinity, 0, depth);

      if (m_shared.stopping)
        break;

      last_score = score;
      last_pv = pv;
      last_depth = depth;

      if (is_main_thread()) {
        if (ctrl.check_soft_termination(stats(), depth))
          break;
        print_info();
      }
    }

    m_shared.stop();

    if (is_main_thread()) {
      print_info();
      m_shared.output->bestmove(last_pv.pv.empty() ? Move::none() : last_pv.pv[0]);
    }
  }

  template<typename Node, typename Controls>
  auto Search::search(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, i32 ply, i32 depth) -> Score {
    if (depth <= 0)
      return qsearch<Node>(ctrl, position, pv, alpha, beta, ply);

    stats().nodes.fetch_add(1, std::memory_order_relaxed);
    if (!Node::is_root && is_main_thread() && ctrl.check_hard_termination(stats())) [[unlikely]] {
      m_shared.stop();
      return 0;
    }

    if (!Node::is_root) {
      if (const auto score = position.is_fifty_move_draw(ply))
        return *score;

      if (position.is_repetition(m_hash_stack, m_hash_waterline))
        return 0;
    }

    if (ply >= max_depth)
      return eval(position);

    const tt::LookupResult tte = tt_load(position, ply);

    if constexpr (!Node::is_pv) {
      if (tte.is_some() && tte.depth >= depth && [&] {
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
    }

    const bool is_in_check = position.is_in_check();

    if (!Node::is_pv && !is_in_check) {
      const i32 static_eval = eval(position);

      // Reduced futility pruning
      if (depth <= 6 && static_eval - 128 * depth >= beta) {
        return static_eval;
      }

      // Beta Multi-Probcut
      if (static_eval >= beta && depth >= 7) {
        const i32 r = 1 + depth / 2;
        const i32 margin = 128 + depth * 10;
        const i32 bound = beta + margin;
        const i32 score = search<node::NonPv>(ctrl, position, pv, bound - 1, bound, ply, depth - r);
        if (score >= bound) {
          return beta;
        }
      }

      // Alpha Multi-Probcut
      if (static_eval < alpha && depth >= 7) {
        const i32 r = 1 + depth / 2;
        const i32 margin = 128 + depth * 10;
        const i32 bound = alpha - margin;
        const i32 score = search<node::NonPv>(ctrl, position, pv, bound, bound + 1, ply, depth - r);
        if (score <= bound) {
          return alpha;
        }
      }
    }

    MovePicker moves {*this, position, tte.move};

    MoveList fail_low_quiets;
    MoveList fail_low_noisies;

    Score best_score = score::none;
    Move best_move = Move::none();
    tt::Bound bound = tt::Bound::upper_bound;
    u32 move_count = 0;

    for (Move mv = moves.next(); mv.is_some(); mv = moves.next()) {
      move_count++;

      const Position child_position = position.move(mv);
      m_hash_stack.push_back(child_position.hash());
      rose_defer {
        m_hash_stack.pop_back();
      };

      Line child_pv {};
      Score score = score::none;
      if (depth >= 3 && move_count >= 3) {
        const i32 log2_depth = std::bit_width(static_cast<u32>(depth)) - 1;
        const i32 log2_move_count = std::bit_width(static_cast<u32>(move_count)) - 1;

        i32 reduction = 3072 + 256 * log2_depth * log2_move_count;

        const i32 lmr_depth = std::clamp(depth - reduction / 1024, 0, depth - 1);

        score = -search<node::NonPv>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ply + 1, lmr_depth);

        if (score > alpha && lmr_depth < depth - 1) {
          score = -search<node::NonPv>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ply + 1, depth - 1);
        }
      } else if (!Node::is_pv || move_count > 1) {
        score = -search<node::NonPv>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ply + 1, depth - 1);
      }
      if (Node::is_pv && (move_count == 1 || score > alpha)) {
        score = -search<node::Pv>(ctrl, child_position, child_pv, -beta, -alpha, ply + 1, depth - 1);
      }

      if (m_shared.stopping)
        return 0;

      if (score > best_score) {
        best_score = score;

        if (score > alpha) {
          bound = tt::Bound::exact;
          alpha = score;
          best_move = mv;

          if constexpr (Node::is_pv)
            pv.write(mv, std::move(child_pv));

          if (score >= beta) {
            bound = tt::Bound::lower_bound;
            break;
          }
        }
      }

      if (mv != best_move) {
        if (mv.capture()) {
          fail_low_noisies.push_back(mv);
        } else {
          fail_low_quiets.push_back(mv);
        }
      }
    }

    if (best_score == score::none) {
      return position.is_in_check() ? score::mated(ply) : 0;
    }

    if (best_move.is_some()) {
      const Color stm = position.stm();

      const i32 quiet_bonus = 150 * depth - 75;
      const i32 quiet_malus = 75 * depth - 30;

      if (!best_move.capture()) {
        m_quiet_history.update(stm, best_move, quiet_bonus);
        for (const Move quiet : fail_low_quiets) {
          m_quiet_history.update(stm, quiet, -quiet_malus);
        }
      }
    }

    tt_store(position,
             ply,
             tt::LookupResult {
               .depth = depth,
               .bound = bound,
               .score = best_score,
               .move = best_move,
             });

    return best_score;
  }

  template<typename Node, typename Controls>
  auto Search::qsearch(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, i32 ply) -> Score {
    stats().nodes.fetch_add(1, std::memory_order_relaxed);
    if (!Node::is_root && is_main_thread() && ctrl.check_hard_termination(stats())) [[unlikely]] {
      m_shared.stop();
      return 0;
    }

    if (!Node::is_root) {
      if (const auto score = position.is_fifty_move_draw(ply))
        return *score;

      if (position.is_repetition(m_hash_stack, m_hash_waterline))
        return 0;
    }

    if (ply >= max_depth)
      return eval(position);

    const Score static_eval = eval(position);
    if (static_eval >= beta) {
      return static_eval;
    }
    alpha = std::max(alpha, static_eval);

    MovePicker moves {*this, position, Move::none()};
    moves.skip_quiet();

    Score best_score = static_eval;
    Move best_move = Move::none();
    tt::Bound bound = tt::Bound::upper_bound;

    for (Move mv = moves.next(); mv.is_some(); mv = moves.next()) {
      const Position child_position = position.move(mv);
      m_hash_stack.push_back(child_position.hash());
      rose_defer {
        m_hash_stack.pop_back();
      };

      Line child_pv {};
      const Score score = -qsearch<typename Node::next>(ctrl, child_position, child_pv, -beta, -alpha, ply + 1);

      if (m_shared.stopping)
        return 0;

      if (score > best_score) {
        best_score = score;

        if (score > alpha) {
          bound = tt::Bound::exact;
          alpha = score;
          best_move = mv;
          if constexpr (Node::is_pv)
            pv.write(mv, std::move(child_pv));

          if (score >= beta) {
            bound = tt::Bound::lower_bound;
            break;
          }
        }
      }
    }

    return best_score;
  }

  auto Search::eval(const Position& position) -> Score {
    return nnue::evaluate(position);
  }

  auto Search::tt_load(const Position& position, i32 ply) -> tt::LookupResult {
    return m_shared.transposition_table.load(position.hash(), ply);
  }

  auto Search::tt_store(const Position& position, i32 ply, tt::LookupResult lr) -> void {
    m_shared.transposition_table.store(position.hash(), ply, lr);
  }

}  // namespace rose
