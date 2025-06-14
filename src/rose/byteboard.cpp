#include "rose/byteboard.h"

#include <bit>
#include <print>

#include "rose/common.h"
#include "rose/square.h"
#include "rose/util/types.h"

namespace rose {

  auto Byteboard::dumpRaw(u64 mask) const -> void {
    const auto r = std::bit_cast<std::array<u8, 64>>(z);
    for (int flipped_rank = 0; flipped_rank < 8; flipped_rank++) {
      for (int file = 0; file < 8; file++) {
        const int rank = 7 - flipped_rank;
        const Square sq = Square::fromFileAndRank(file, rank);
        if ((mask >> sq.raw) & 1) {
          std::print("{:02x} ", r[sq.raw]);
        } else {
          std::print("-- ");
        }
      }
      std::print("\n");
    }
  }

  auto Byteboard::dumpSq(u64 mask) const -> void {
    const auto r = std::bit_cast<std::array<Square, 64>>(z);
    for (int flipped_rank = 0; flipped_rank < 8; flipped_rank++) {
      for (int file = 0; file < 8; file++) {
        const int rank = 7 - flipped_rank;
        const Square sq = Square::fromFileAndRank(file, rank);
        if ((mask >> sq.raw) & 1) {
          std::print("{} ", r[sq.raw]);
        } else {
          std::print("-- ");
        }
      }
      std::print("\n");
    }
  }

  auto Wordboard::dumpRaw(u64 mask) const -> void {
    const auto r = toMailbox();
    for (int flipped_rank = 0; flipped_rank < 8; flipped_rank++) {
      for (int file = 0; file < 8; file++) {
        const int rank = 7 - flipped_rank;
        const Square sq = Square::fromFileAndRank(file, rank);
        if ((mask >> sq.raw) & 1) {
          std::print("{:04x} ", r[sq.raw]);
        } else {
          std::print("---- ");
        }
      }
      std::print("\n");
    }
  }

  auto dumpRaysSq(v512 z, u64 mask) -> void {
    const auto r = std::bit_cast<std::array<Square, 64>>(z);
    for (int ray = 0; ray < 8; ray++) {
      for (int i = 1; i < 8; i++) {
        const int index = ray * 8 + i;
        if ((mask >> index) & 1) {
          std::print("{} ", r[index]);
        } else {
          std::print("-- ");
        }
      }
      std::print("\n");
    }
    std::print("knight: ");
    for (int ray = 0; ray < 8; ray++) {
      const int index = ray * 8;
      if ((mask >> index) & 1) {
        std::print("{} ", r[index]);
      } else {
        std::print("-- ");
      }
    }
    std::print("\n");
  }

  auto dumpRaysRaw(v512 z, u64 mask) -> void {
    const auto r = std::bit_cast<std::array<u8, 64>>(z);
    for (int ray = 0; ray < 8; ray++) {
      for (int i = 1; i < 8; i++) {
        const int index = ray * 8 + i;
        if ((mask >> index) & 1) {
          std::print("{:02x} ", r[index]);
        } else {
          std::print("-- ");
        }
      }
      std::print("\n");
    }
    std::print("knight: ");
    for (int ray = 0; ray < 8; ray++) {
      const int index = ray * 8;
      if ((mask >> index) & 1) {
        std::print("{:02x} ", r[index]);
      } else {
        std::print("-- ");
      }
    }
    std::print("\n");
  }

} // namespace rose
