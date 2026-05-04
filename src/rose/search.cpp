#include "rose/search.hpp"

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/move_picker.hpp"
#include "rose/movegen.hpp"
#include "rose/nnue/nnue.hpp"
#include "rose/score.hpp"
#include "rose/search_control.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/time.hpp"

#include <atomic>
#include <fmt/format.h>
#include <memory>
#include <thread>
#include <variant>

namespace rose {

  constexpr i32 max_depth = 250;

  auto SearchShared::reset() -> void {
    // TODO: Implement TT
  }

  auto SearchShared::set_hash_size(int mb) -> void {
    // TODO: Implement TT
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
    // TODO: Histories etc
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
      const Score score = search(ctrl, m_root, pv, -score::infinity, score::infinity, 0, depth);

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

  template<typename Controls>
  auto Search::search(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, i32 ply, i32 depth) -> Score {
    const bool is_root = ply == 0;

    stats().nodes.fetch_add(1, std::memory_order_relaxed);
    if (!is_root && is_main_thread() && ctrl.check_hard_termination(stats(), depth)) [[unlikely]] {
      m_shared.stop();
      return 0;
    }

    if (depth <= 0 || ply >= max_depth)
      return eval(position);

    MovePicker moves {*this, position};

    Score best_score = score::none;

    for (Move m = moves.next(); m.is_some(); m = moves.next()) {
      const Position child_position = position.move(m);

      Line child_pv {};
      const Score score = -search(ctrl, child_position, child_pv, -beta, -alpha, ply + 1, depth - 1);

      if (m_shared.stopping)
        return 0;

      if (score > best_score) {
        best_score = score;
        pv.write(m, std::move(child_pv));

        if (score > alpha)
          alpha = score;
        if (score >= beta)
          break;
      }
    }

    if (best_score == score::none) {
      return position.is_in_check() ? score::mated(ply) : 0;
    }
    return best_score;
  }

  auto Search::eval(const Position& position) -> Score {
    return nnue::evaluate(position);
  }

}  // namespace rose
