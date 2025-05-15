#pragma once

#include <atomic>
#include <barrier>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "rose/game.h"
#include "rose/line.h"
#include "rose/movegen.h"
#include "rose/search_control.h"
#include "rose/search_stats.h"
#include "rose/tt.h"
#include "rose/util/types.h"

namespace rose {

  struct SearchShared {
    explicit SearchShared(int thread_count) : idle_barrier(1 + thread_count), started_barrier(1 + thread_count), stats(thread_count) {}
    std::shared_mutex mutex{};
    std::atomic_bool stop;
    std::barrier<> idle_barrier;
    std::barrier<> started_barrier;
    controls::Any ctrl;

    std::vector<SearchStats> stats;
    PrecompMoveGenInfo movegen_precomp;

    tt::TT transposition_table{tt::default_hash_size_mb};

    inline auto totalNodes() const -> u64 {
      u64 nodes = 0;
      for (const SearchStats &s : stats)
        nodes += s.nodes.load(std::memory_order::relaxed);
      return nodes;
    }
  };

  struct Search {
  private:
    friend struct MovePicker;

    usize m_id;
    SearchShared &m_shared;

    std::jthread m_thread;
    Game m_game;

  public:
    Search(usize id, SearchShared &shared);

    auto reset() -> void;

    auto isMainThread() const -> bool { return m_id == 0; }

    auto setGame(const Game &g) -> void;

    auto requestStart() -> void;
    auto requestQuit() -> void { m_thread.request_stop(); }

  private:
    auto threadMain(std::stop_token quit) -> void;

    inline auto requestStop() -> void { return m_shared.stop.store(true, std::memory_order::relaxed); }
    inline auto hasStopped() const -> bool { return m_shared.stop.load(std::memory_order::relaxed); }
    inline auto stats() -> SearchStats & { return m_shared.stats[m_id]; }

    template <typename Controls> auto searchRoot(const Controls &ctrl) -> void;

    inline auto isDraw(bool is_in_check, i32 ply) -> std::optional<i32>;
    inline auto ttLoad(int ply) const -> tt::LookupResult;
    inline auto ttStore(int ply, tt::LookupResult lr) -> void;

    template <typename Controls> auto search(const Controls &ctrl, Line &pv, i32 alpha, i32 beta, i32 depth, i32 ply) -> i32;
  };

} // namespace rose
