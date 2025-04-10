#include <format>
#include <print>
#include <string_view>
#include <tuple>
#include <vector>

#include "rose/position.h"
#include "rose/util/assert.h"

using namespace rose;

auto roundtrip() -> void {
  const std::vector<std::string_view> cases{{
      "7r/3r1p1p/6p1/1p6/2B5/5PP1/1Q5P/1K1k4 b - - 0 38",
      "r3k2r/pp1bnpbp/1q3np1/3p4/3N1P2/1PP1Q2P/P1B3P1/RNB1K2R b KQkq - 5 15",
      "8/5p2/1kn1r1n1/1p1pP3/6K1/8/4R3/5R2 b - - 9 60",
      "r4rk1/1Bp1qppp/2np1n2/1pb1p1B1/4P1b1/P1NP1N2/1PP1QPPP/R4RK1 b - b6 1 11",
  }};
  for (std::string_view fen : cases) {
    const Position position = Position::parse(fen).value();
    const std::string result = std::format("{}", position);
    rose_assert(result == fen, "{} != {}", fen, result);
  }
}

auto pinned() -> void {
  const std::vector<std::tuple<std::string_view, u64>> cases{{
      {"3r4/B2R2q1/1b1Rn3/2N5/Rr1K1rPP/2BP4/2NP1P2/q2r4 w - - 0 1", 0x0000000400040000},
      {"r7/8/8/4b3/8/8/8/K3R1b1 w - - 0 1", 0},
      {"8/8/5b2/4n3/3P4/8/1K6/8 w - - 0 1", 0},
  }};
  for (const auto [fen, expected_pins] : cases) {
    const Position position = Position::parse(fen).value();
    rose_assert(expected_pins == position.pinned());
  }
}

auto main() -> int {
  roundtrip();
  pinned();
  return 0;
}
