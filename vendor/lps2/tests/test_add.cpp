#include "tests.hpp"

#include <array>
#include <bit>
#include <lps2/lps2.hpp>

using namespace lps2;

int main() {
  vector<i32, 16> a { 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4 };
  vector<i32, 16> b { 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7 };

  vector<i32, 16> c = a + b;
  vector<i32, 16> expected { 5, 7, 9, 11, 5, 7, 9, 11, 5, 7, 9, 11, 5, 7, 9, 11 };

  REQUIRE((std::bit_cast<std::array<i32, 16>>(c.raw) == std::bit_cast<std::array<i32, 16>>(expected.raw)), "should be equal");
}
