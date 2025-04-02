#include <print>

#include "rose/byte_board.h"
#include "rose/position.h"
#include "rose/square.h"
#include "rose/util/types.h"

using namespace rose;

auto main(int argc, char **argv) -> int {
  Byteboard bb;
  for (int i = 0; i < 128; i++)
    bb.r[i] = i;
  bb.dumpRaw();
  std::print("\n");

  const auto position = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  position.value().prettyPrint();
  std::print("fen: {}\n", position.value());

  return 0;
}
