#pragma once

#include <print>
#include <variant>

#include "rose/depth.h"
#include "rose/search_stats.h"
#include "rose/util/types.h"

namespace rose::controls {

  struct None {
    time::TimePoint start_time;

    auto elapsed() const -> time::Duration { return time::Clock::now() - start_time; }
    constexpr auto checkSoftTermination(const SearchStats &stats, Depth current_depth) const -> bool { return false; }
    constexpr auto checkHardTermination(const SearchStats &stats, Depth current_depth) const -> bool { return false; }

    auto dump() const -> void { std::print("# search control: infinite\n"); }
  };

  struct Time {
    time::TimePoint start_time;
    time::Duration hard_time;
    time::Duration soft_time;

    auto elapsed() const -> time::Duration { return time::Clock::now() - start_time; }
    auto checkSoftTermination(const SearchStats &stats, Depth current_depth) const -> bool { return soft_time <= elapsed(); }
    auto checkHardTermination(const SearchStats &stats, Depth current_depth) const -> bool {
      return stats.nodes.load(std::memory_order::relaxed) % 1024 == 0 && hard_time <= elapsed();
    }

    auto dump() const -> void {
      std::print("# search control: hardtime {} softtime {}\n", time::cast<time::Milliseconds>(hard_time), time::cast<time::Milliseconds>(soft_time));
    }
  };

  struct DataGen {
    time::TimePoint start_time;
    int hard_nodes;
    int soft_nodes;

    auto elapsed() const -> time::Duration { return time::Clock::now() - start_time; }
    auto checkSoftTermination(const SearchStats &stats, Depth current_depth) const -> bool {
      return soft_nodes <= stats.nodes.load(std::memory_order::relaxed);
    }
    auto checkHardTermination(const SearchStats &stats, Depth current_depth) const -> bool {
      return hard_nodes <= stats.nodes.load(std::memory_order::relaxed);
    }

    auto dump() const -> void { std::print("# search control: nodes {} softnodes {}\n", hard_nodes, soft_nodes); }
  };

  struct All {
    time::TimePoint start_time;
    std::optional<time::Duration> hard_time;
    std::optional<time::Duration> soft_time;
    std::optional<int> hard_nodes;
    std::optional<int> soft_nodes;
    std::optional<int> depth;

    auto elapsed() const -> time::Duration { return time::Clock::now() - start_time; }
    auto checkSoftTermination(const SearchStats &stats, Depth current_depth) const -> bool {
      if (soft_time && *soft_time <= elapsed())
        return true;
      if (soft_nodes && *soft_nodes <= stats.nodes.load(std::memory_order::relaxed))
        return true;
      if (depth && *depth <= current_depth.rtz())
        return true;
      return false;
    }
    auto checkHardTermination(const SearchStats &stats, Depth current_depth) const -> bool {
      if (hard_time && *hard_time <= elapsed())
        return true;
      if (hard_nodes && *hard_nodes <= stats.nodes.load(std::memory_order::relaxed))
        return true;
      return false;
    }

    auto dump() const -> void {
      std::print("# search control:");
      if (hard_time)
        std::print(" hardtime {}", time::cast<time::Milliseconds>(*hard_time));
      if (soft_time)
        std::print(" softtime {}", time::cast<time::Milliseconds>(*soft_time));
      if (hard_nodes)
        std::print(" nodes {}", *hard_nodes);
      if (soft_nodes)
        std::print(" softnodes {}", *soft_nodes);
      if (depth)
        std::print(" depth {}", *depth);
      std::print("\n");
    }
  };

  using Any = std::variant<controls::None, controls::Time, controls::DataGen, controls::All>;

} // namespace rose::controls
