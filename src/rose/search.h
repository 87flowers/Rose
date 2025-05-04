#pragma once

#include <atomic>
#include <barrier>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "rose/game.h"
#include "rose/util/types.h"

namespace rose {

  struct SearchShared {
    explicit SearchShared(int thread_count) : idle_barrier(1 + thread_count), started_barrier(1 + thread_count) {}
    std::shared_mutex mutex{};
    std::atomic_flag stop;
    std::barrier<> idle_barrier;
    std::barrier<> started_barrier;
  };

  struct Search {
  private:
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
  };

} // namespace rose
