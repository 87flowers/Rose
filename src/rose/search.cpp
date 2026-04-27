#include "rose/search.hpp"

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/movegen.hpp"
#include "rose/search_control.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/time.hpp"

#include <fmt/format.h>
#include <memory>
#include <random>
#include <thread>
#include <variant>

namespace rose {

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

  auto calc_ctrl(time::TimePoint start_time, const SearchLimit& limits) -> controls::Any {
    rose_unused(start_time, limits);
    return controls::None {};
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

        if (is_main_thread()) {
          const auto ctrl = calc_ctrl(m_shared.search_start_time, m_shared.search_main_limits);
          (void)m_shared.started_barrier.arrive();
          std::visit(
            [this](const auto& ctrl) {
              this->search_root(ctrl);
            },
            ctrl);
        } else {
          (void)m_shared.started_barrier.arrive();
          search_root<controls::None>({});
        }
      } break;
      }
    }
  }

  template<typename Controls>
  auto Search::search_root(const Controls& ctrl) -> void {
    rose_unused(ctrl);
    static std::mt19937_64 prng_engine {};
    MoveList moves;
    MoveGen movegen {m_root};
    movegen.generate_moves(moves);

    if (moves.size() == 0) {
      fmt::print("bestmove null\n");
    } else {
      std::uniform_int_distribution<usize> rand {0, moves.size() - 1};
      const Move m = moves[rand(prng_engine)];

      Line line;
      line.write(m);

      m_shared.output->info(EngineOutput::Info {
        .depth = 1,
        .score = 0,
        .time = time::Clock::now() - m_shared.search_start_time,
        .nodes = 1,
        .pv = line,
      });
      m_shared.output->bestmove(m);
    }
  }

}  // namespace rose
