#include "rose/perft.h"

#include <print>

#include "rose/common.h"
#include "rose/move.h"
#include "rose/movegen.h"
#include "rose/position.h"
#include "rose/util/types.h"

namespace rose::perft {

  template <bool print> static auto core(const PrecompMoveGenInfo &movegen_precomp, const Position &position, usize depth) -> usize {
    if (depth == 0)
      return 1;

    usize result = 0;

    MoveList moves;
    MoveGen movegen{position, movegen_precomp};
    movegen.generateMoves(moves);

    if (!print && depth == 1)
      return moves.size();

    for (Move m : moves) {
      const Position new_position = position.move(m);
      const usize child = core<false>(movegen_precomp, new_position, depth - 1);
      if constexpr (print) {
        std::print("{}: {}\n", m, child);
      }
      result += child;
    }

    return result;
  }

  auto value(const Position &position, usize depth) -> usize {
    const PrecompMoveGenInfo movegen_precomp{position};
    return core<false>(movegen_precomp, position, depth);
  }

  auto run(const Position &position, usize depth) -> void {
    const auto start = time::Clock::now();
    const PrecompMoveGenInfo movegen_precomp{position};
    const usize total = core<true>(movegen_precomp, position, depth);
    const auto end = time::Clock::now();

    const time::FloatSeconds elapsed = end - start;

    std::print("total: {}\n", total);
    std::print("perft to depth {} complete in {:.1f}ms ({:.1f} Mnps)\n", depth, elapsed.count() * 1000, total / (1'000'000 * elapsed.count()));
  }

} // namespace rose::perft
