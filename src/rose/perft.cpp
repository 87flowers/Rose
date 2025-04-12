#include "rose/perft.h"

#include <print>

#include "rose/common.h"
#include "rose/move.h"
#include "rose/movegen.h"
#include "rose/position.h"
#include "rose/util/types.h"

namespace rose::perft {

  template <bool print> static auto core(const Position &position, usize depth) -> usize {
    if (depth == 0)
      return 1;

    usize result = 0;

    MoveList moves;
    MoveGen movegen{position};
    movegen.generateMoves(moves);

    for (Move m : moves) {
      const Position new_position = position.move(m);
      if (!new_position.isValid())
        continue;

      const usize child = core<false>(new_position, depth - 1);
      if constexpr (print) {
        std::print("{}: {}\n", m, child);
      }
      result += child;
    }

    return result;
  }

  auto value(const Position &position, usize depth) -> usize { return core<false>(position, depth); }

  auto run(const Position &position, usize depth) -> void {
    const auto start = time::Clock::now();
    const usize total = core<true>(position, depth);
    const auto end = time::Clock::now();

    const time::FloatSeconds elapsed = end - start;

    std::print("total: {}\n", total);
    std::print("perft to depth {} complete in {:.1f}ms ({:.1f} Mnps)\n", depth, elapsed.count() * 1000, total / (1'000'000 * elapsed.count()));
  }

} // namespace rose::perft
