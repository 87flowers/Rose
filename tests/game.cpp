#include <print>

#include "rose/game.h"
#include "rose/move.h"
#include "rose/util/assert.h"

using namespace rose;

auto move(Game &g, std::string_view move_str) -> void {
  g.move(Move::parse(move_str, g.position()).value());
  std::print("{:016x} : {}\n", g.hash(), move_str);
  rose_assert(g.position().hash() == g.hash());
  rose_assert(g.position().hash() == g.position().calcHashSlow());
}
auto moveAndSet(Game &g, std::string_view move_str) -> void {
  move(g, move_str);
  g.setHashWaterline();
}

auto repeatInSearch() -> void {
  Game g;
  g.reset();
  g.setPositionStartpos();
  std::print("{:016x} : startpos\n", g.hash());
  g.setHashWaterline();
  rose_assert(g.isRepetition() == false);
  move(g, "g1f3");
  rose_assert(g.isRepetition() == false);
  move(g, "g8f6");
  rose_assert(g.isRepetition() == false);
  move(g, "f3g1");
  rose_assert(g.isRepetition() == false);
  move(g, "f6g8");
  rose_assert(g.isRepetition() == true);
  move(g, "g1f3");
  rose_assert(g.isRepetition() == true);
  move(g, "g8f6");
  rose_assert(g.isRepetition() == true);
  move(g, "f3g1");
  rose_assert(g.isRepetition() == true);
  move(g, "f6g8");
  rose_assert(g.isRepetition() == true);
}

auto repeatInPrehistory() -> void {
  Game g;
  g.reset();
  g.setPositionStartpos();
  std::print("{:016x} : startpos\n", g.hash());
  g.setHashWaterline();
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "g1f3");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "g8f6");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "f3g1");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "f6g8");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "g1f3");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "g8f6");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "f3g1");
  rose_assert(g.isRepetition() == false);
  moveAndSet(g, "f6g8");
  rose_assert(g.isRepetition() == true);
}

auto main() -> int {
  repeatInSearch();
  repeatInPrehistory();
  return 0;
}
