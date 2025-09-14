#include "rose/cmd/bench.h"
#include "rose/engine.h"
#include "rose/game.h"
#include "rose/util/types.h"

using namespace rose;

auto main() -> int {
  bench::run(true);
  return 0;
}
