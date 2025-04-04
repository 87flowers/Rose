#pragma once

#include <array>
#include <print>
#include <tuple>

#include "rose/common.h"
#include "rose/geometry.h"
#include "rose/square.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  union alignas(64) Byteboard {
    v512 z;
    std::array<Place, 64> m;
    std::array<u8, 64> r{};

    constexpr Byteboard() = default;

    template <PieceType ptype> auto bitboardFor(Color color) const -> u64 {
      v512 board = z;
      if constexpr (ptype == PieceType::k || ptype == PieceType::r) {
        constexpr u8 place_mask = Place::color_mask | Place::ptype_mask;
        board = vec::and8(board, v512::broadcast8(place_mask));
      }
      const u8 expected_place = Place::fromColorAndPtype(color, ptype).raw;
      return vec::eq8(board, v512::broadcast8(expected_place));
    }

    auto getOccupiedBitboard() const -> u64 { return z.nonzero8(); }
    auto getColorBitboard(Color color) const -> u64 { return (z.msb8() ^ color.toBitboard()) & getOccupiedBitboard(); }

    auto getSuperpieceAttacks(Square sq) const -> std::tuple<v512, v512, u64> {
      const auto [raycoords, coordvalid] = geometry::superpieceRays(sq);
      const v512 rayplaces = vec::permute8(raycoords, z);
      const u64 occupied = rayplaces.nonzero8();
      const u64 rayattacks = geometry::superpieceAttacks(occupied, coordvalid);
      return {raycoords, rayplaces, rayattacks};
    }

    auto dumpRaw(u64 mask = ~u64{0}) const -> void {
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

    auto dumpSq(u64 mask = ~u64{0}) const -> void {
      for (int flipped_rank = 0; flipped_rank < 8; flipped_rank++) {
        for (int file = 0; file < 8; file++) {
          const int rank = 7 - flipped_rank;
          const Square sq = Square::fromFileAndRank(file, rank);
          if ((mask >> sq.raw) & 1) {
            std::print("{} ", Square{r[sq.raw]});
          } else {
            std::print("-- ");
          }
        }
        std::print("\n");
      }
    }
  };

  inline auto dumpRaysSq(v512 z, u64 mask = ~u64{0}) -> void {
    Byteboard bb;
    bb.z = z;
    for (int ray = 0; ray < 9; ray++) {
      const int maxi = ray < 8 ? 7 : 8;
      for (int i = 0; i < maxi; i++) {
        const int index = ray * 7 + i;
        if ((mask >> index) & 1) {
          std::print("{} ", Square{bb.r[index]});
        } else {
          std::print("-- ");
        }
      }
      std::print("\n");
    }
  }

  inline auto dumpRaysRaw(v512 z, u64 mask = ~u64{0}) -> void {
    Byteboard bb;
    bb.z = z;
    for (int ray = 0; ray < 9; ray++) {
      const int maxi = ray < 8 ? 7 : 8;
      for (int i = 0; i < maxi; i++) {
        const int index = ray * 7 + i;
        if ((mask >> index) & 1) {
          std::print("{:02x} ", bb.r[index]);
        } else {
          std::print("-- ");
        }
      }
      std::print("\n");
    }
  }

} // namespace rose
