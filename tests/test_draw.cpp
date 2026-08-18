#include "rose/common.hpp"
#include "rose/game.hpp"
#include "rose/is_draw.hpp"
#include "rose/move.hpp"
#include "rose/util/assert.hpp"

#include <fmt/format.h>

using namespace rose;

static usize hash_waterline = 0;

auto set_waterline(Game& g) -> void {
  hash_waterline = g.hash_stack().size() - 1;
}

auto is_repetition(Game& g) -> bool {
  return draw::is_repetition(g.position(), g.hash_stack(), hash_waterline);
}

auto move(Game& g, std::string_view move_str) -> void {
  g.move(Move::parse(move_str, MoveFormat::frc, g.position()).value());
  fmt::print("{:016x} : {}\n", g.hash(), move_str);
  rose_assert(g.hash() == g.position().calc_hashes_slow().full);
}

auto move_and_set_waterline(Game& g, std::string_view move_str) -> void {
  move(g, move_str);
  set_waterline(g);
}

auto repeat_in_search() -> void {
  Game g;
  g.reset();
  fmt::print("{:016x} : startpos\n", g.hash());
  set_waterline(g);
  rose_assert(is_repetition(g) == false);
  move(g, "g1f3");
  rose_assert(is_repetition(g) == false);
  move(g, "g8f6");
  rose_assert(is_repetition(g) == false);
  move(g, "f3g1");
  rose_assert(is_repetition(g) == false);
  move(g, "f6g8");
  rose_assert(is_repetition(g) == true);
  move(g, "g1f3");
  rose_assert(is_repetition(g) == true);
  move(g, "g8f6");
  rose_assert(is_repetition(g) == true);
  move(g, "f3g1");
  rose_assert(is_repetition(g) == true);
  move(g, "f6g8");
  rose_assert(is_repetition(g) == true);
}

auto repeat_in_prehistory() -> void {
  Game g;
  g.reset();
  fmt::print("{:016x} : startpos\n", g.hash());
  set_waterline(g);
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "g1f3");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "g8f6");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "f3g1");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "f6g8");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "g1f3");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "g8f6");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "f3g1");
  rose_assert(is_repetition(g) == false);
  move_and_set_waterline(g, "f6g8");
  rose_assert(is_repetition(g) == true);
}

auto main() -> int {
  repeat_in_search();
  repeat_in_prehistory();
  return 0;
}
