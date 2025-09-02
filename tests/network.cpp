#include <print>
#include <string_view>

#include "rose/eval/network.h"
#include "rose/position.h"

using namespace rose;

static auto evaluate(std::string_view fen) -> i32 {
  const Position pos = Position::parse(fen).value();
  return eval::evaluate(eval::accumulatorsFromPosition(pos), pos.activeColor());
}

static auto printEvaluation(std::string_view fen) -> void { std::print("{}: {}\n", fen, evaluate(fen)); }

auto main() -> int {
  const eval::Network &net = eval::defaultNetwork();

  // std::print("hl bias: {}\n", net.accumulator_biases);
  std::print("output bias: {}\n", net.output_bias);

  printEvaluation("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  printEvaluation("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  return 0;
}
