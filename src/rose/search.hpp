#pragma once

#include "rose/engine.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/hash.hpp"
#include "rose/history.hpp"
#include "rose/line.hpp"
#include "rose/move.hpp"
#include "rose/position.hpp"
#include "rose/score.hpp"
#include "rose/search_stats.hpp"
#include "rose/tt.hpp"
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
    explicit SearchShared(int thread_count, usize initial_tt_size, std::shared_ptr<EngineOutput> output) :
        idle_barrier(1 + thread_count),
        started_barrier(1 + thread_count),
        output(output),
        stats(thread_count),
        transposition_table(initial_tt_size) {
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
    std::vector<SearchStats> stats;
    tt::TT transposition_table;

    auto reset() -> void;

    auto set_hash_size(int mb) -> void;
    auto set_output(std::shared_ptr<EngineOutput> output) -> void;

    auto send_ping() -> void;
    auto send_quit() -> void;
    auto send_go(time::TimePoint start_time, const SearchLimit& limits, const Game& g) -> void;

    auto stop() -> void;

    auto total_nodes() -> u64 {
      u64 total = 0;
      for (const SearchStats& s : stats)
        total += s.nodes.load(std::memory_order_relaxed);
      return total;
    }
  };

  struct Search {
  private:
    friend struct MovePicker;

    int m_id;
    SearchShared& m_shared;

    std::jthread m_thread;

    Position m_root;
    std::vector<Move> m_move_stack;
    std::vector<Hash> m_hash_stack;
    usize m_hash_waterline;

    QuietHistory m_quiet_history;

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
    auto stats() -> SearchStats& {
      return m_shared.stats[m_id];
    }

    auto thread_main() -> void;

    template<typename Controls>
    auto search_root(const Controls& ctrl) -> void;

    template<typename Controls>
    auto search(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, i32 ply, i32 depth) -> Score;

    auto eval(const Position& position) -> Score;

    auto tt_load(const Position& position, i32 ply) -> tt::LookupResult;
    auto tt_store(const Position& position, i32 ply, tt::LookupResult lr) -> void;
  };

}  // namespace rose
