#pragma once

#include <memory>
#include <vector>

#include "rose/game.h"
#include "rose/search.h"
#include "rose/util/types.h"

namespace rose {

  struct Engine {
  private:
    std::vector<Search> m_searches;
    std::unique_ptr<SearchShared> m_shared;

  public:
    Engine();
    ~Engine();

    auto reset() -> void;

    auto setThreadCount(int thread_count) -> void;

    auto setGame(const Game &g) -> void;

    auto isReady() -> void;

    auto runSearch() -> void;

  private:
    auto startAllThreads() -> void;
    auto quitAllThreads() -> void;
  };

} // namespace rose
