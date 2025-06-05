#include "rose/cmd/perft.h"

#include <print>

#include "rose/common.h"
#include "rose/move.h"
#include "rose/movegen.h"
#include "rose/position.h"
#include "rose/util/types.h"

namespace rose::perft {

  template <bool print, bool bulk> static auto core(const PrecompMoveGenInfo &movegen_precomp, const Position &position, usize depth) -> u64 {
    if (depth == 0)
      return 1;

    u64 result = 0;

    MoveList moves;
    MoveGen movegen{position, movegen_precomp};
    movegen.generateMoves(moves);

    if (!print && depth == 1 && bulk)
      return moves.size();

    for (Move m : moves) {
      const Position new_position = position.move(m);
      const u64 child = core<false, bulk>(movegen_precomp, new_position, depth - 1);
      if constexpr (print) {
        std::print("{}: {}\n", m, child);
      }
      result += child;
    }

    return result;
  }

  auto value(const Position &position, usize depth) -> u64 {
    const PrecompMoveGenInfo movegen_precomp{position};
    return core<false, true>(movegen_precomp, position, depth);
  }

  auto run(const Position &position, usize depth, bool bulk) -> void {
    const auto start = time::Clock::now();
    const PrecompMoveGenInfo movegen_precomp{position};
    const u64 total = bulk ? core<true, true>(movegen_precomp, position, depth) : core<true, false>(movegen_precomp, position, depth);
    const auto end = time::Clock::now();

    const time::FloatSeconds elapsed = end - start;

    std::print("total: {}\n", total);
    std::print("perft to depth {} complete in {:.1f}ms ({:.1f} Mnps)\n", depth, elapsed.count() * 1000, time::nps<f64>(total, elapsed) / 1'000'000);
  }

} // namespace rose::perft
