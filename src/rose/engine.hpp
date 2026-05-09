#pragma once

#include "rose/common.hpp"
#include "rose/util/time.hpp"

#include <memory>
#include <vector>

namespace rose {

  struct EngineOutput;
  struct Game;
  struct Search;
  struct SearchLimit;
  struct SearchShared;

  struct Engine {
  private:
    std::vector<std::unique_ptr<Search>> m_searches;
    std::unique_ptr<SearchShared> m_shared;
    std::shared_ptr<EngineOutput> m_output;
    usize m_tt_size;

  public:
    Engine();
    ~Engine();

    auto reset() -> void;

    auto set_hash_size(int mb) -> void;
    auto set_thread_count(int thread_count) -> void;
    auto set_output(std::shared_ptr<EngineOutput> output) -> void;

    auto run_search(time::TimePoint start_time, const SearchLimit& limits, const Game& g) -> void;

    auto wait() -> void;
    auto stop() -> void;
  };

}  // namespace rose
