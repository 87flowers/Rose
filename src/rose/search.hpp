#pragma once

#include "rose/engine.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/hash.hpp"
#include "rose/move.hpp"
#include "rose/position.hpp"
#include "rose/util/time.hpp"

#include <atomic>
#include <barrier>
#include <memory>
#include <optional>
#include <thread>

namespace rose {

  struct SearchLimit {
    bool has_time = false;
    std::optional<int> wtime;
    std::optional<int> btime;
    std::optional<int> winc;
    std::optional<int> binc;
    std::optional<int> movestogo;
    std::optional<int> movetime;

    bool has_other = false;
    std::optional<int> hard_nodes;
    std::optional<int> soft_nodes;
    std::optional<int> depth;
  };

  enum EngineMessage {
    ping,
    quit,
    go,
  };

  struct SearchShared {
    explicit SearchShared(int thread_count, std::shared_ptr<EngineOutput> output) :
        idle_barrier(1 + thread_count),
        started_barrier(1 + thread_count),
        output(output) {
    }

    std::shared_ptr<EngineOutput> output;

    // Synchronization
    std::atomic_bool stopping;
    std::barrier<> idle_barrier;
    std::barrier<> started_barrier;

    // UCI -> Search Thread Communication
    std::atomic<EngineMessage> engine_message;
    time::TimePoint search_start_time;
    SearchLimit search_main_limits;
    const Game* search_game = nullptr;

    // Shared Search Data
    // TODO

    auto reset() -> void;

    auto set_hash_size(int mb) -> void;
    auto set_output(std::shared_ptr<EngineOutput> output) -> void;

    auto send_ping() -> void;
    auto send_quit() -> void;
    auto send_go(time::TimePoint start_time, const SearchLimit& limits, const Game& g) -> void;

    auto stop() -> void;
  };

  struct Search {
  private:
    int m_id;
    SearchShared& m_shared;

    std::jthread m_thread;

    Position m_root;
    std::vector<Move> m_move_stack;
    std::vector<Hash> m_hash_stack;
    usize m_hash_waterline;

  public:
    Search(int id, SearchShared& shared) :
        m_id(id),
        m_shared(shared) {
    }

    auto reset() -> void;
    auto launch() -> void;

    auto is_main_thread() -> bool {
      return m_id == 0;
    }

  private:
    auto thread_main() -> void;

    template<typename Controls>
    auto search_root(const Controls& ctrl) -> void;
  };

}  // namespace rose
