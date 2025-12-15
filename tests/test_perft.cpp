#include "rose/cmd/perft.hpp"
#include "rose/config.hpp"
#include "rose/position.hpp"
#include "rose/util/assert.hpp"

#include <fmt/format.h>
#include <string_view>
#include <tuple>
#include <vector>

using namespace rose;

auto main() -> int {
  std::vector<std::tuple<std::string_view, std::vector<u64>>> cases {{
    {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      {{1, 20, 400, 8902, 197281, 4865609, 119060324}},
    },
    {
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      {{1, 48, 2039, 97862, 4085603, 193690690}},
    },
    {
      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      {{1, 14, 191, 2812, 43238, 674624, 11030083}},
    },
    {
      "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
      {{1, 6, 264, 9467, 422333, 15833292}},
    },
    {
      "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
      {{1, 44, 1486, 62379, 2103487, 89941194}},
    },
    {
      "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
      {{1, 46, 2079, 89890, 3894594, 164075551}},
    },
    {
      "1bbrnkqr/pp1p1ppp/2p1p3/1n6/5P2/3Q4/PPPPP1PP/NBBRNK1R w HDhd - 2 9",
      {{1, 36, 891, 31075, 781792, 26998966}},
    },
    {
      "2r1kr2/8/8/8/8/8/8/1R2K1R1 w GBfc - 0 1",
      {{1, 22, 501, 11459, 264663, 6236222, 149271720}},
    },
  }};

  config::frc = true;

  for (const auto [fen, results] : cases) {
    const Position position = Position::parse(fen).value();
    fmt::print("{}:\n", position);

    for (usize depth = 0; depth < results.size(); depth++) {
      const u64 value = perft::value(position, depth);
      fmt::print("{}: {}\n", depth, value);
      rose_assert(value == results[depth]);
    }
  }

  return 0;
}
