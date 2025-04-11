#include <print>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "rose/movegen.h"
#include "rose/position.h"
#include "rose/util/assert.h"
#include "rose/util/vec.h"

using namespace rose;

template <> struct std::hash<Move> {
  std::size_t operator()(const Move &m) const noexcept { return std::hash<u16>{}(m.raw); }
};

auto doubleCheck() -> void {
  std::vector<std::tuple<std::string_view, std::vector<std::string_view>>> cases{{
      {"3q3k/6b1/8/8/3K4/2P1P3/8/8 w - - 0 1", {"d4c5", "d4c4", "d4e4"}},
  }};
  for (const auto [fen, should_movelist] : cases) {
    std::print("{}\n", fen);

    const Position position = Position::parse(fen).value();

    std::unordered_set<Move> should_moves;
    for (const auto m : should_movelist)
      should_moves.emplace(Move::parse(m).value());

    MoveList got_movelist;
    MoveGen movegen{position};
    movegen.generateMoves(got_movelist);

    std::unordered_set<Move> got_moves;
    for (const auto m : got_movelist) {
      got_moves.insert(m);
      std::print("{}\n", m);
    }

    rose_assert(got_moves == should_moves);
  }
}

auto main() -> int {
  doubleCheck();
  return 0;
}
