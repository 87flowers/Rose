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
        board = board & v512::broadcast8(place_mask);
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

    auto dumpRaw(u64 mask = ~u64{0}) const -> void;
    auto dumpSq(u64 mask = ~u64{0}) const -> void;

    constexpr auto operator==(const Byteboard &other) const -> bool { return r == other.r; };
  };

  union alignas(64) Wordboard {
    std::array<v512, 2> z;
    std::array<u16, 64> r{};

    constexpr Wordboard() = default;

    auto dumpRaw(u64 mask = ~u64{0}) const -> void;

    constexpr auto operator==(const Wordboard &other) const -> bool { return r == other.r; };
  };

  auto dumpRaysSq(v512 z, u64 mask = ~u64{0}) -> void;
  auto dumpRaysRaw(v512 z, u64 mask = ~u64{0}) -> void;

} // namespace rose
