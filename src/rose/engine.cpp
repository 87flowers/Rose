#include "rose/engine.hpp"

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/engine_output_null.hpp"
#include "rose/game.hpp"
#include "rose/search.hpp"
#include "rose/util/assert.hpp"

#include <memory>
#include <tuple>

namespace rose {

  Engine::Engine() :
      m_output(std::make_shared<EngineOutputNull>()) {
    set_thread_count(1);
  }

  Engine::~Engine() {
    if (m_shared) {
      m_shared->stop();
      m_shared->send_quit();
    }
  }

  auto Engine::reset() -> void {
    wait();
    m_shared->reset();
    for (const auto& search : m_searches)
      search->reset();
  }

  auto Engine::set_hash_size(int mb) -> void {
    rose_assert(mb > 0);
    m_shared->set_hash_size(mb);
  }

  auto Engine::set_thread_count(int thread_count) -> void {
    rose_assert(thread_count > 0);

    if (m_shared) {
      m_shared->stop();
      m_shared->send_quit();
      m_searches.clear();
    }

    m_shared = std::make_unique<SearchShared>(thread_count, m_output);

    for (int i = 0; i < thread_count; i++)
      m_searches.emplace_back(std::make_unique<Search>(i, *m_shared));
    for (const auto& search : m_searches)
      search->launch();
  }

  auto Engine::set_output(std::shared_ptr<EngineOutput> output) -> void {
    wait();
    m_output = output;
    m_shared->set_output(output);
  }

  auto Engine::run_search(time::TimePoint start_time, const SearchLimit& limits, const Game& g) -> void {
    m_shared->send_go(start_time, limits, g);
  }

  auto Engine::wait() -> void {
    m_shared->send_ping();
  }

  auto Engine::stop() -> void {
    m_shared->stop();
  }

}  // namespace rose
