#include "rose/cmd/bench.h"

#include <array>
#include <string_view>

#include "rose/config.h"
#include "rose/engine.h"
#include "rose/engine_output_uci.h"
#include "rose/game.h"
#include "rose/position.h"
#include "rose/uci.h"
#include "rose/util/defer.h"

namespace rose::bench {

  static constexpr int bench_depth = 15;
  static constexpr std::array<std::string_view, 40> bench_fens{{
      "1k1r2r1/pp3p2/3p4/1b2q3/3QP1pp/8/PB3PPP/2R1R1K1 w - - 0 1",
      "1R6/5pk1/4r3/5pPp/1p6/8/1PK5/8 b - - 0 1",
      "1rkn3r/p1p4p/p3R3/3Q2b1/3p4/8/PPP2PPP/6K1 w - - 0 1",
      "2k1K3/pp6/2p5/3p4/8/8/PPq5/8 w - - 0 1",
      "2Q5/1p6/3k4/8/1p6/1p6/3Q1P2/4K3 b - - 0 1",
      "3k1qr1/pb2b3/1p1pNnp1/1B1p4/8/8/PPKN1PP1/4R2R b - - 0 1",
      "3n4/2R5/p2p2k1/1p6/1P1PbP2/P5P1/5K1P/8 b - - 0 1",
      "3r1rk1/p4p2/1n2p3/6Qp/4N3/P2qP2P/1P3PP1/2R3K1 b - - 0 1",
      "3R4/5k2/7K/7P/8/3b4/8/8 b - - 0 1",
      "3rk2r/pp2qpp1/4p3/2p1Pn1p/2RnN2P/1P1Q1NP1/P4P2/5RK1 w k - 0 1",
      "4k3/1p3pp1/4p3/3pP3/8/1n3b2/2pr1PN1/4RK2 w - - 0 1",
      "5k2/1R3p2/4p1p1/6Pp/4KP1P/8/8/r7 w - - 0 1",
      "6k1/1bq3b1/p6p/P1p3P1/2P1pn2/R6r/2BQ1P1P/3R2K1 w - - 0 1",
      "6k1/6p1/7p/5N2/1r2K1PP/8/8/q7 w - - 0 1",
      "6r1/p1p5/2p5/4P3/2P1k3/1PB5/P5p1/6K1 b - - 0 1",
      "8/2k5/8/3RK3/1N6/8/8/8 w - - 0 1",
      "8/3k1ppp/p3p3/1r2P3/3B2P1/3n1K1P/PPR2P2/8 b - - 0 1",
      "8/3nk3/3r2p1/1P3p2/2Q4P/5PK1/8/8 w - - 0 1",
      "8/3nkBp1/p6r/p1r1p2p/5P2/1P5P/P1P5/3RR1K1 b - - 0 1",
      "8/3Q3k/8/8/4K1N1/8/8/8 b - - 0 1",
      "8/8/pp1k1pp1/2pP4/2P1KP2/6PP/8/8 b - - 0 1",
      "q6k/8/5p2/5Q1P/6P1/8/5P1K/8 b - - 0 1",
      "r1b2rk1/1ppqbppp/p1n1pn2/8/2NP4/2N1P1P1/PP3PBP/R1BQ1RK1 b - - 0 1",
      "r1bq1rk1/1ppn1pb1/pn2p1pp/8/2pPP3/2N1BNP1/PPQ2PBP/3RR1K1 w - - 0 1",
      "r1bq1rk1/4nppp/p2p4/1pp1n3/4PB2/1BP1Q3/PP3PPP/RN3RK1 b - - 0 1",
      "r1bqk1nr/pppp1pp1/2n1p2p/8/1bBPP3/2N2N2/PPP2PPP/R1BQK2R b KQkq - 0 1",
      "r1bqkb1r/ppp2ppp/2n2n2/8/3P4/2N1B3/PP3PPP/R2QKBNR w KQkq - 0 1",
      "r1bqn1k1/pp1pprbp/6p1/n3PN2/8/2N1B3/PPP2PPP/R2QK2R b KQ - 0 1",
      "r1br1k2/p3b1pp/5p2/1Nn5/2Bpp3/6BP/PPPK1PP1/R2R4 w - - 0 1",
      "r1q2rk1/3bbp1n/p2p2p1/nppPp1Pp/4PP1P/2P5/PPB4N/R1BQRNK1 b - - 0 1",
      "r2q1r1k/1bp3b1/1p1p4/p1nPpnpB/2P4p/2N2P1P/PPQN2P1/R3R1BK w - - 0 1",
      "r2q2k1/1b2Qppp/p7/1p6/1Pn5/6NP/P1B2PP1/4R1K1 w - - 0 1",
      "r3k2r/ppqb1pp1/4p1np/3pP3/8/1QP2N2/PP2BPPP/R4RK1 b kq - 0 1",
      "r3kb1r/2qb1ppp/p3p3/n2pP3/Pp1NnP2/3QB3/NPP1B1PP/R4R1K w kq - 0 1",
      "r4k1r/p6p/1n1N2p1/1p3p2/1P6/2P1b3/2B3PP/1K1R1R2 w - - 0 1",
      "r5k1/1bp2ppp/3p1q2/p1p1r3/2Pp1N2/1P3PP1/P2QR2P/4R1K1 w - - 0 1",
      "r6k/1p2b1r1/p2p1p1p/4p2B/4P1PK/3PB3/PP3P2/R4R2 w - - 0 1",
      "rnbk1b1r/1p1ppp1p/p7/1Np5/3PP2p/2P1PN2/Pq4PP/R2QKB1R w KQ - 0 1",
      "rnbq1rk1/ppp2ppp/4pn2/3p4/3P1PP1/2PB4/PP3P1P/RN1QK1NR b KQ - 0 1",
      "rnbqkb1r/pp2pp2/2p2np1/6Pp/3P4/5B2/PPP2P1P/RNBQK1NR b KQkq - 0 1",
  }};

  auto run(bool print_output) -> void {
    Engine engine;
    Game game;

    if (print_output)
      engine.setOutput(std::make_shared<rose::EngineOutputUci>());

    const time::TimePoint start_time = time::Clock::now();
    const SearchLimit limit = [] {
      SearchLimit limit;
      limit.has_other = true;
      limit.depth = bench_depth;
      return limit;
    }();

    engine.isReady();

    int positions_benched = 0;
    u64 nodes_total = 0;
    for (const auto fen : bench_fens) {
      game.reset();
      engine.reset();

      const Position position = Position::parse(fen).value();
      game.setPosition(position);
      engine.setGame(game);

      positions_benched++;
      std::print("{}/{} ...\r", positions_benched, bench_fens.size());
      std::fflush(stdout);

      engine.runSearch(time::Clock::now(), limit);
      engine.isReady();
      nodes_total += engine.lastSearchTotalNodes();
    }

    const time::FloatSeconds elapsed = time::Clock::now() - start_time;
    std::print("bench results:\n");
    std::print("nodes: {} nodes\n", nodes_total);
    std::print("time:  {} milliseconds\n", time::cast<time::Milliseconds>(elapsed).count());
    std::print("nps:   {} nps\n", time::nps<u64>(nodes_total, elapsed));
    std::fflush(stdout);
  }

} // namespace rose::bench
