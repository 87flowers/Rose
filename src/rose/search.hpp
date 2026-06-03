#pragma once

#include "rose/engine.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/hash.hpp"
#include "rose/history.hpp"
#include "rose/limits.hpp"
#include "rose/line.hpp"
#include "rose/move.hpp"
#include "rose/node_type.hpp"
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

  struct SearchStack {
    Move move = Move::none();
    Move excluded = Move::none();
    Move killer = Move::none();
    Score static_eval = score::none;
    ContinuationHistorySubtable* conthist = nullptr;
  };

  struct SearchData {
    QuietHistory quiet_history;
    NoisyHistory noisy_history;
    ContinuationHistory continuation_history;
  };

  struct SearchBase {
    virtual ~SearchBase() = default;
    virtual auto reset() -> void = 0;
    virtual auto launch() -> void = 0;
  };

  template<eval::concepts::State Evaluation>
  struct Search final : public SearchBase {
  private:
    friend struct MovePicker;

    int m_id;
    SearchShared& m_shared;

    std::jthread m_thread;

    Position m_root;
    std::vector<Move> m_move_stack;
    std::vector<Hash> m_hash_stack;
    usize m_hash_waterline;

    inline static constexpr usize search_stack_offset = 8;
    inline static constexpr usize search_stack_safety = 8;
    std::array<SearchStack, max_depth + search_stack_offset + search_stack_safety> m_search_stack;

    Evaluation m_evaluation;

    SearchData m_sd;
    std::optional<i32> m_nmr_ply;

  public:
    template<typename Network>
    Search(int id, SearchShared& shared, const Network& network) :
        m_id(id),
        m_shared(shared),
        m_evaluation(network) {
    }

    ~Search() final = default;

    auto reset() -> void final;
    auto launch() -> void final;

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

    template<NodeType expected, bool is_root = false, typename Controls>
    auto search(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, SearchStack* ss, i32 ply, i32 depth) -> Score;
    template<NodeType leaf_expected, typename Controls>
    auto qsearch(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, SearchStack* ss, i32 ply) -> Score;

    auto eval(const Position& position) -> Score;

    auto tt_load(const Position& position, i32 ply) -> tt::LookupResult;
    auto tt_store(const Position& position, i32 ply, tt::LookupResult lr) -> void;

    [[nodiscard]] auto make_move(SearchStack* ss, const Position& position, Move mv) -> Position;
    [[nodiscard]] auto make_null_move(SearchStack* ss, const Position& position) -> Position;
    auto unmake_move(SearchStack* ss) -> void;
  };

}  // namespace rose
