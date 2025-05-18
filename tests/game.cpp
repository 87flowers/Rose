#include <print>

#include "rose/game.h"
#include "rose/move.h"
#include "rose/util/assert.h"

using namespace rose;

static usize hash_waterline = 0;
auto setHashWaterline(Game &g) -> void { hash_waterline = g.hashStack().size() - 1; }
auto isRepetition(Game &g) -> bool { return g.position().isRepetition(g.hashStack(), hash_waterline); }

auto move(Game &g, std::string_view move_str) -> void {
  g.move(Move::parse(move_str, g.position()).value());
  std::print("{:016x} : {}\n", g.hash(), move_str);
  rose_assert(g.position().hash() == g.hash());
  rose_assert(g.position().hash() == g.position().calcHashSlow());
}

auto moveAndSet(Game &g, std::string_view move_str) -> void {
  move(g, move_str);
  setHashWaterline(g);
}

auto repeatInSearch() -> void {
  Game g;
  g.reset();
  g.setPositionStartpos();
  std::print("{:016x} : startpos\n", g.hash());
  setHashWaterline(g);
  rose_assert(isRepetition(g) == false);
  move(g, "g1f3");
  rose_assert(isRepetition(g) == false);
  move(g, "g8f6");
  rose_assert(isRepetition(g) == false);
  move(g, "f3g1");
  rose_assert(isRepetition(g) == false);
  move(g, "f6g8");
  rose_assert(isRepetition(g) == true);
  move(g, "g1f3");
  rose_assert(isRepetition(g) == true);
  move(g, "g8f6");
  rose_assert(isRepetition(g) == true);
  move(g, "f3g1");
  rose_assert(isRepetition(g) == true);
  move(g, "f6g8");
  rose_assert(isRepetition(g) == true);
}

auto repeatInPrehistory() -> void {
  Game g;
  g.reset();
  g.setPositionStartpos();
  std::print("{:016x} : startpos\n", g.hash());
  setHashWaterline(g);
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "g1f3");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "g8f6");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "f3g1");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "f6g8");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "g1f3");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "g8f6");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "f3g1");
  rose_assert(isRepetition(g) == false);
  moveAndSet(g, "f6g8");
  rose_assert(isRepetition(g) == true);
}

auto main() -> int {
  repeatInSearch();
  repeatInPrehistory();
  return 0;
}
