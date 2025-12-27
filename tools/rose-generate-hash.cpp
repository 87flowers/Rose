#include "rose/common.hpp"

#include <array>
#include <fmt/format.h>
#include <random>
#include <string_view>

using namespace rose;

auto main() -> int {
  std::mt19937_64 prng {87};

  const std::array<std::string_view, 16> piece_table_headers {{
    "(000) Invalid",
    "(001) White King",
    "(002) White Pawn",
    "(003) White Knight",
    "(004) Invalid",
    "(005) White Bishop",
    "(006) White Rook",
    "(007) White Queen",
    "(010) Invalid",
    "(011) Black King",
    "(012) Black Pawn",
    "(013) Black Knight",
    "(014) Invalid",
    "(015) Black Bishop",
    "(016) Black Rook",
    "(017) Black Queen",
  }};

  fmt::print("#include \"rose/hash.hpp\"\n");
  fmt::print("\n");
  fmt::print("#include \"rose/common.hpp\"\n");
  fmt::print("\n");
  fmt::print("#include <array>\n");
  fmt::print("\n");
  fmt::print("namespace rose::hash {{\n");
  fmt::print("\n");
  fmt::print("  const std::array<std::array<Hash, 64>, 16> piece_table {{{{\n");
  for (const std::string_view header : piece_table_headers) {
    const bool invalid = header.contains("Invalid");
    fmt::print("    // {}\n", header);
    fmt::print("    {{{{\n");
    for (int i = 0; i < 8; i++) {
      fmt::print("     ");
      for (int j = 0; j < 8; j++) {
        const u64 h = invalid ? 0 : prng();
        fmt::print(" 0x{:016x},", h);
      }
      fmt::print("  //\n");
    }
    fmt::print("    }}}},\n");
  }
  fmt::print("  }}}};\n");
  fmt::print("\n");
  fmt::print("  const std::array<Hash, 8> enpassant_table {{{{\n");
  fmt::print("   ");
  for (int i = 0; i < 8; i++) {
    fmt::print(" 0x{:016x},", prng());
  }
  fmt::print("\n");
  fmt::print("  }}}};\n");
  fmt::print("\n");
  fmt::print("  const std::array<Hash, 8> castle_table {{{{\n");
  fmt::print("   ");
  for (int i = 0; i < 8; i++) {
    fmt::print(" 0x{:016x},", prng());
  }
  fmt::print("\n");
  fmt::print("  }}}};\n");
  fmt::print("\n");
  fmt::print("}}  // namespace rose::hash\n");
}
