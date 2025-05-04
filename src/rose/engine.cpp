#include "rose/engine.h"

#include <mutex>
#include <tuple>

#include "rose/game.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose {

  Engine::Engine() { setThreadCount(1); }

  Engine::~Engine() { quitAllThreads(); }

  auto Engine::reset() -> void {
    const std::unique_lock _{m_shared->mutex};
    m_active_color = Color::white;
    for (const auto &search : m_searches)
      search->reset();
  }

  auto Engine::setThreadCount(int thread_count) -> void {
    rose_assert(thread_count > 0);

    if (m_shared) {
      const std::unique_lock _{m_shared->mutex};
      quitAllThreads();
    }

    m_shared = std::make_unique<SearchShared>(thread_count);

    for (usize i = 0; i < thread_count; i++)
      m_searches.emplace_back(std::make_unique<Search>(i, *m_shared));
    for (const auto &search : m_searches)
      search->requestStart();
  }

  auto Engine::setGame(const Game &g) -> void {
    const std::unique_lock _{m_shared->mutex};
    m_active_color = g.position().activeColor();
    for (const auto &search : m_searches)
      search->setGame(g);
  }

  auto Engine::isReady() -> void {
    const std::unique_lock _{m_shared->mutex};
    // We are ready if we are able to acquire exclusive access to the mutex
    return;
  }

  auto Engine::runSearch(time::TimePoint start_time, const SearchLimit &limits) -> void {
    {
      const std::unique_lock _{m_shared->mutex};

      m_shared->stop.store(false);

      if (limits.has_time && !limits.has_other) [[likely]] {
        controls::Time ctrl;

        ctrl.start_time = start_time;
        std::tie(ctrl.hard_time, ctrl.soft_time) = calcTime(limits);

        m_shared->ctrl = ctrl;
      } else if (limits.has_time || limits.has_other) [[unlikely]] {
        controls::All ctrl;

        ctrl.start_time = start_time;
        if (limits.has_time)
          std::tie(ctrl.hard_time, ctrl.soft_time) = calcTime(limits);
        ctrl.hard_nodes = limits.hard_nodes;
        ctrl.soft_nodes = limits.soft_nodes;
        ctrl.depth = limits.depth;

        m_shared->ctrl = ctrl;
      } else {
        m_shared->ctrl = controls::None{};
      }
    }
    startAllThreads();
  }

  auto Engine::stop() -> void { m_shared->stop.store(true); }

  auto Engine::calcTime(const SearchLimit &limits) const -> std::tuple<time::Duration, time::Duration> {
    constexpr time::Milliseconds margin_ms{100};
    constexpr time::Milliseconds zero_ms{0};

    const auto remaining_time = m_active_color == Color::white ? limits.wtime : limits.btime;
    const auto increment = m_active_color == Color::white ? limits.winc : limits.binc;

    const time::Milliseconds remaining_time_ms{remaining_time.value_or(0)};
    const time::Milliseconds increment_ms{increment.value_or(0)};
    const int movestogo = limits.movestogo.value_or(20);

    time::Milliseconds safe_remaining_ms = std::max(remaining_time_ms - margin_ms, zero_ms);

    if (limits.movetime) [[unlikely]] {
      const time::Milliseconds movetime_ms{*limits.movetime};
      if (!remaining_time && !increment)
        return {movetime_ms, movetime_ms};
      safe_remaining_ms = std::min(safe_remaining_ms, movetime_ms);
    }

    const time::Milliseconds hard_limit = std::min(safe_remaining_ms / movestogo * 7 + increment_ms / 3, safe_remaining_ms);
    const time::Milliseconds soft_limit = std::min(safe_remaining_ms / movestogo + increment_ms / 3, safe_remaining_ms);

    return {hard_limit, soft_limit};
  }

  auto Engine::startAllThreads() -> void {
    m_shared->idle_barrier.arrive_and_wait();
    m_shared->started_barrier.arrive_and_wait();
  }

  auto Engine::quitAllThreads() -> void {
    for (const auto &search : m_searches)
      search->requestQuit();
    m_shared->idle_barrier.arrive_and_wait();
    m_searches.clear();
  }

} // namespace rose
