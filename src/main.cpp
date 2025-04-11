#include <print>

#include "rose/byteboard.h"
#include "rose/geometry.h"
#include "rose/position.h"
#include "rose/square.h"
#include "rose/util/types.h"

using namespace rose;

auto main(int argc, char **argv) -> int {
  const Position position = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").value();

  position.board().dumpRaw();
  std::print("\n");
  position.pieceListSq(Color::white).dump();
  position.attackTable(Color::white).dumpRaw();
  std::print("\n");
  position.pieceListSq(Color::black).dump();
  position.attackTable(Color::black).dumpRaw();
  std::print("\n");
  position.printAttackTable();

  return 0;
}
