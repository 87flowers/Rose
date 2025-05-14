#include <print>

#include "rose/move.h"
#include "rose/position.h"
#include "rose/tt.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

using namespace rose;

auto basic() -> void {
  tt::TT transposition_table{8};

  const u64 hash = 0xc7e672b3132ccc8a;
  const int ply = 3;
  const int depth1 = 7;
  const int score1 = 42;
  const int depth2 = 10;
  const int score2 = 43;
  const Move move = Move::parse("e2e4", Position::startpos()).value();

  rose_assert(transposition_table.load(hash, ply).bound == tt::Bound::none);

  transposition_table.store(hash, ply,
                            {
                                .depth = depth1,
                                .bound = tt::Bound::exact,
                                .score = score1,
                                .move = move,
                            });

  const tt::LookupResult tte1 = transposition_table.load(hash, ply);
  rose_assert(tte1.depth == depth1, "{}", tte1.depth);
  rose_assert(tte1.bound == tt::Bound::exact);
  rose_assert(tte1.score == score1);
  rose_assert(tte1.move == move);

  transposition_table.store(hash, ply,
                            {
                                .depth = depth2,
                                .bound = tt::Bound::upper_bound,
                                .score = score2,
                                .move = move,
                            });

  const tt::LookupResult tte2 = transposition_table.load(hash, ply);
  rose_assert(tte2.depth == depth2);
  rose_assert(tte2.bound == tt::Bound::upper_bound);
  rose_assert(tte2.score == score2);
  rose_assert(tte2.move == move);
}

auto main() -> int {
  basic();
  return 0;
}
