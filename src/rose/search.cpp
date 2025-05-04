#include "rose/search.h"

#include <mutex>
#include <thread>

#include "rose/game.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose {

  Search::Search(usize id, SearchShared &shared) : m_id(id), m_shared(shared) { reset(); }

  auto Search::reset() -> void { m_game.reset(); }

  auto Search::setGame(const Game &g) -> void {
    m_game = g;
    m_game.setHashWaterline();
  }

  auto Search::requestStart() -> void {
    rose_assert(!m_thread.joinable());
    m_thread = std::jthread([this](std::stop_token quit) { this->threadMain(quit); });
  }

  auto Search::threadMain(std::stop_token quit) -> void {
    while (true) {
      m_shared.idle_barrier.arrive_and_wait();

      if (quit.stop_requested()) [[unlikely]]
        break;

      std::shared_lock _{m_shared.mutex};
      (void)m_shared.started_barrier.arrive();

      m_game.printGameRecord();
    }
  }

} // namespace rose
