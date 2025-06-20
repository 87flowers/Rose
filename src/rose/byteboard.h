#pragma once

#include <array>
#include <bit>
#include <cstring>
#include <print>
#include <tuple>

#include "rose/common.h"
#include "rose/geometry.h"
#include "rose/square.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  struct alignas(64) Byteboard {
    v512 z;

    constexpr Byteboard() = default;

    auto toMailbox() const -> std::array<Place, 64> { return std::bit_cast<std::array<Place, 64>>(z); }
    auto read(Square sq) const -> Place {
      Place value;
      std::memcpy(&value, reinterpret_cast<const char *>(&z) + sq.raw * sizeof(u8), sizeof(u8));
      return value;
    }
    auto write(Square sq, Place value) -> void { std::memcpy(reinterpret_cast<char *>(&z) + sq.raw * sizeof(u8), &value, sizeof(u8)); }

    template <PieceType ptype> auto bitboardFor(Color color) const -> u64 {
      const u8 expected_place = Place::fromColorAndPtypeAndId(color, ptype, 0).raw;
      return vec::eq8(z & v512::broadcast8(0xF0), v512::broadcast8(expected_place));
    }

    auto getEmptyBitboard() const -> u64 { return z.zero8(); }
    auto getOccupiedBitboard() const -> u64 { return z.nonzero8(); }
    auto getColorBitboard(Color color) const -> u64 { return (z.msb8() ^ ~color.toBitboard()) & getOccupiedBitboard(); }

    auto getSuperpieceAttacks(Square sq) const -> std::tuple<v512, v512, u64> {
      const auto [raycoords, coordvalid] = geometry::superpieceRays(sq);
      const v512 rayplaces = vec::permute8(raycoords, z);
      const u64 occupied = rayplaces.nonzero8();
      const u64 rayattacks = geometry::superpieceAttacks(occupied, coordvalid);
      return {raycoords, rayplaces, rayattacks};
    }

    auto dumpRaw(u64 mask = ~u64{0}) const -> void;
    auto dumpSq(u64 mask = ~u64{0}) const -> void;

    constexpr auto operator==(const Byteboard &other) const -> bool { return z == other.z; };
  };

  struct alignas(64) Wordboard {
    std::array<v512, 2> z;

    constexpr Wordboard() = default;
    constexpr Wordboard(v512 a, v512 b) : z({a, b}) {}

    auto toMailbox() const -> std::array<u16, 64> { return std::bit_cast<std::array<u16, 64>>(z); }
    auto read(Square sq) const -> u16 {
      u16 value;
      std::memcpy(&value, reinterpret_cast<const char *>(z.data()) + sq.raw * sizeof(u16), sizeof(u16));
      return value;
    }

    auto dumpRaw(u64 mask = ~u64{0}) const -> void;

    auto getAttackedBitboard() const -> u64 { return vec::concatlo64(z[0].nonzero16(), z[1].nonzero16()); }
    auto getBitboardFor(u16 piece_mask) const -> u64 {
      v512 pm = v512::broadcast16(piece_mask);
      return vec::concatlo64(vec::test16(z[0], pm), vec::test16(z[1], pm));
    }

    constexpr auto operator==(const Wordboard &other) const -> bool { return z == other.z; };

    friend auto operator&(const Wordboard &a, const Wordboard &b) -> Wordboard { return {a.z[0] & b.z[0], a.z[1] & b.z[1]}; }
  };

  auto dumpRaysSq(v512 z, u64 mask = ~u64{0}) -> void;
  auto dumpRaysRaw(v512 z, u64 mask = ~u64{0}) -> void;

} // namespace rose
