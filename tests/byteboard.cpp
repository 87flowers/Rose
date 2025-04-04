#include <print>
#include <string_view>

#include "rose/byteboard.h"
#include "rose/position.h"
#include "rose/square.h"
#include "rose/util/assert.h"

using namespace rose;

auto getSuperpieceAttacks() -> void {
  {
    const auto position = Position::parse("3rk3/4q1q1/1b1P4/4p1n1/r2K3q/8/2N1n3/3R2B1 w - - 0 1").value();
    const Square sq = Square::parse("d4").value();
    const auto [raycoords, coordvalid] = geometry::superpieceRays(sq);
    const auto [raycoords2, rayplaces, rayattacks] = position.board().getSuperpieceAttacks(sq);

    rose_assert(raycoords == raycoords2);

    dumpRaysSq(raycoords, coordvalid);
    std::print("\n");
    dumpRaysRaw(rayplaces, rayattacks);
    std::print("\n");
  }
  {
    const auto position = Position::parse("8/8/8/8/8/8/8/K6r w - - 0 1").value();
    const Square sq = Square::parse("a1").value();
    const auto [raycoords, coordvalid] = geometry::superpieceRays(sq);
    const auto [raycoords2, rayplaces, rayattacks] = position.board().getSuperpieceAttacks(sq);

    rose_assert(raycoords == raycoords2);

    dumpRaysSq(raycoords, coordvalid);
    std::print("\n");
    dumpRaysRaw(rayplaces, rayattacks);
    std::print("\n");
  }
}

auto main() -> int {
  getSuperpieceAttacks();
  return 0;
}
