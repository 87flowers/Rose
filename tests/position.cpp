#include <format>
#include <print>
#include <string_view>
#include <tuple>
#include <vector>

#include "rose/config.h"
#include "rose/position.h"
#include "rose/util/assert.h"

using namespace rose;

auto roundtripClassical() -> void {
  const std::vector<std::string_view> cases{{
      "7r/3r1p1p/6p1/1p6/2B5/5PP1/1Q5P/1K1k4 b - - 0 38",
      "r3k2r/pp1bnpbp/1q3np1/3p4/3N1P2/1PP1Q2P/P1B3P1/RNB1K2R b KQkq - 5 15",
      "8/5p2/1kn1r1n1/1p1pP3/6K1/8/4R3/5R2 b - - 9 60",
      "r4rk1/1Bp1qppp/2np1n2/1pb1p1B1/4P1b1/P1NP1N2/1PP1QPPP/R4RK1 b - b6 1 11",
  }};
  config::frc = false;
  for (std::string_view fen : cases) {
    const Position position = Position::parse(fen).value();
    const std::string result = std::format("{}", position);
    rose_assert(result == fen, "{} != {}", fen, result);
  }
}

auto roundtripDfrc() -> void {
  const std::vector<std::string_view> cases{{
      "2r1kr2/8/8/8/8/8/8/1R2K1R1 w GBfc - 0 1",
      "rkr5/8/8/8/8/8/8/5RKR w HFca - 0 1",
      "2r3kr/8/8/8/8/8/8/2KRR3 w h - 3 2",
      "5rkr/8/8/8/8/8/8/RKR5 w CAhf - 0 1",
      "3rkr2/8/8/8/8/8/8/R3K2R w HAfd - 0 1",
      "4k3/8/8/8/8/8/8/4KR2 w F - 0 1",
      "4kr2/8/8/8/8/8/8/4K3 w f - 0 1",
      "4k3/8/8/8/8/8/8/2R1K3 w C - 0 1",
      "2r1k3/8/8/8/8/8/8/4K3 w c - 0 1",
  }};
  config::frc = true;
  for (std::string_view fen : cases) {
    const Position position = Position::parse(fen).value();
    const std::string result = std::format("{}", position);
    rose_assert(result == fen, "{} != {}", fen, result);
  }
}

auto main() -> int {
  roundtripClassical();
  roundtripDfrc();
  return 0;
}
