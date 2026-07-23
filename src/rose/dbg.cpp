#include "rose/dbg.hpp"

#include "fmt/base.h"
#include "rose/common.hpp"

#include <atomic>
#include <cmath>
#include <fmt/format.h>
#include <limits>

namespace rose::dbg {

  namespace {

    struct Hit {
      std::atomic<u64> trues;
      std::atomic<u64> count;

      auto clear() -> void {
        trues = 0;
        count = 0;
      }

      auto add(bool value) -> void {
        trues += value;
        count++;
      }

      auto print(usize slot) -> void {
        const u64 count_ = count.load();
        const u64 trues_ = trues.load();
        const f64 rate = 100.0 * trues_ / count_;
        fmt::print("Hit #{}: Total {} Hits {} Hit Rate (%) {}\n", slot, count_, trues_, rate);
      }
    };

    struct Stats {
      std::atomic<i64> min = std::numeric_limits<i64>::max();
      std::atomic<i64> max = std::numeric_limits<i64>::min();
      std::atomic<u64> count;
      std::atomic<f64> sum;
      std::atomic<f64> sum_sq;

      auto clear() -> void {
        min = std::numeric_limits<i64>::max();
        max = std::numeric_limits<i64>::min();
        count = 0;
        sum = 0;
        sum_sq = 0;
      }

      auto add(i64 value) -> void {
        min = std::min(value, min.load());  // TODO: fetch_min
        max = std::max(value, max.load());  // TODO: fetch_max
        count++;
        sum += value;
        sum_sq += static_cast<f64>(value) * static_cast<f64>(value);
      }

      auto print(usize slot) -> void {
        const i64 max_ = max.load();
        const i64 min_ = min.load();
        const u64 count_ = count.load();
        const f64 sum_ = sum.load();
        const f64 sum_sq_ = sum_sq.load();
        const f64 mean = sum_ / count_;
        const f64 sd = std::sqrt(sum_sq_ / count) - std::pow(sum_ / count, 2.0);
        fmt::print("Stat #{}: Count {} Total {} Mean {} Min {} Max {} StdDev {}\n", slot, count_, sum_, mean, min_, max_, sd);
      }
    };

    std::array<Hit, 32> hits;
    std::array<Stats, 32> stats;

  }  // namespace

  auto dbg_hit(usize slot, bool value) -> void {
    hits[slot].add(value);
  }

  auto dbg_stats(usize slot, i64 value) -> void {
    stats[slot].add(value);
  }

  auto print() -> void {
    for (usize slot = 0; slot < hits.size(); slot++)
      if (hits[slot].count)
        hits[slot].print(slot);
    for (usize slot = 0; slot < stats.size(); slot++)
      if (stats[slot].count)
        stats[slot].print(slot);
  }

  auto clear() -> void {
    for (usize slot = 0; slot < hits.size(); slot++)
      hits[slot].clear();
    for (usize slot = 0; slot < stats.size(); slot++)
      stats[slot].clear();
  }

}  // namespace rose::dbg
