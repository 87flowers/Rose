#pragma once

#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include "rose/common.h"
#include "rose/game.h"
#include "rose/search.h"
#include "rose/util/types.h"

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

  struct Engine {
  private:
    std::vector<std::unique_ptr<Search>> m_searches;
    std::unique_ptr<SearchShared> m_shared;
    Color m_active_color = Color::white;

  public:
    Engine();
    ~Engine();

    auto reset() -> void;

    auto setThreadCount(int thread_count) -> void;

    auto setGame(const Game &g) -> void;

    auto isReady() -> void;

    auto runSearch(time::TimePoint start_time, const SearchLimit &limits) -> void;

    auto stop() -> void;

    auto lastSearchTotalNodes() -> u64;

  private:
    auto calcTime(const SearchLimit &limits) const -> std::tuple<time::Duration, time::Duration>;

    auto startAllThreads() -> void;
    auto quitAllThreads() -> void;
  };

} // namespace rose
