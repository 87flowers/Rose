#include <print>

#include "rose/byteboard.h"
#include "rose/geometry.h"
#include "rose/square.h"
#include "rose/util/types.h"

using namespace rose;

auto main(int argc, char **argv) -> int {
  Byteboard bb;
  const auto sq = Square::parse("e4").value();
  const auto [z, valid] = geometry::superpieceRays(sq);
  bb.z = z;

  std::print("{:02x} {:02x}\n", sq.raw, geometry::internal::expandSq(sq));
  bb.dumpSq(valid);
  std::print("\n");

  return 0;
}
