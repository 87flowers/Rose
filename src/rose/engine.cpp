#include "rose/engine.h"

#include <mutex>

#include "rose/game.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose {

  Engine::Engine() { setThreadCount(1); }

  Engine::~Engine() { quitAllThreads(); }

  auto Engine::reset() -> void {
    const std::unique_lock _{m_shared->mutex};
    for (Search &search : m_searches)
      search.reset();
  }

  auto Engine::setThreadCount(int thread_count) -> void {
    rose_assert(thread_count > 0);

    if (m_shared) {
      const std::unique_lock _{m_shared->mutex};
      quitAllThreads();
    }

    m_shared = std::make_unique<SearchShared>(thread_count);

    for (usize i = 0; i < thread_count; i++)
      m_searches.emplace_back(i, *m_shared);
    for (Search &search : m_searches)
      search.requestStart();
  }

  auto Engine::setGame(const Game &g) -> void {
    const std::unique_lock _{m_shared->mutex};
    for (Search &search : m_searches)
      search.setGame(g);
  }

  auto Engine::isReady() -> void {
    const std::unique_lock _{m_shared->mutex};
    // We are ready if we are able to acquire exclusive access to the mutex
    return;
  }

  auto Engine::runSearch() -> void {
    {
      const std::unique_lock _{m_shared->mutex};
      m_shared->stop.clear();
    }
    startAllThreads();
  }

  auto Engine::startAllThreads() -> void {
    m_shared->idle_barrier.arrive_and_wait();
    m_shared->started_barrier.arrive_and_wait();
  }

  auto Engine::quitAllThreads() -> void {
    for (Search &search : m_searches)
      search.requestQuit();
    m_shared->idle_barrier.arrive_and_wait();
    m_searches.clear();
  }

} // namespace rose
