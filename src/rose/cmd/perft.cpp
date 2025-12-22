#include "rose/cmd/perft.hpp"

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/movegen.hpp"
#include "rose/position.hpp"
#include "rose/util/time.hpp"

#include <fmt/format.h>

namespace rose::perft {

  template<bool print, bool bulk>
  static auto core(const Position& position, usize depth) -> u64 {
    if (depth == 0)
      return 1;

    u64 result = 0;

    MoveList moves;
    MoveGen movegen {position};
    movegen.generate_moves(moves);

    if (!print && depth == 1 && bulk)
      return moves.size();

    for (Move m : moves) {
      const Position new_position = position.move(m);
      const u64 child = core<false, bulk>(new_position, depth - 1);
      if constexpr (print) {
        fmt::print("{}: {}\n", m, child);
      }
      result += child;
    }

    return result;
  }

  auto value(const Position& position, usize depth) -> u64 {
    return core<false, true>(position, depth);
  }

  auto run(const Position& position, usize depth, bool bulk) -> void {
    const auto start = time::Clock::now();
    const u64 total = bulk ? core<true, true>(position, depth) : core<true, false>(position, depth);
    const auto end = time::Clock::now();

    const time::FloatSeconds elapsed = end - start;

    fmt::print("total: {}\n", total);
    fmt::print("perft to depth {} complete in {:.1f}ms ({:.1f} Mnps)\n", depth, elapsed.count() * 1000, time::nps<f64>(total, elapsed) / 1'000'000);
  }

}  // namespace rose::perft
