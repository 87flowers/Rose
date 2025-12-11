#pragma once

#include "common.hpp"
#include "square.hpp"

#include <bit>

namespace rose {

  struct Bitboard {
    u64 raw = 0;

    constexpr Bitboard() = default;

    constexpr explicit Bitboard(u64 raw) :
        raw(raw) {
    }

    auto empty() const -> bool {
      return raw == 0;
    }

    auto lsb() const -> Square {
      return Square { static_cast<u8>(std::countr_zero(m_raw)) };
    }

    bool operator==(const Bitboard&) const = default;
  };

  constexpr auto Color::to_bitboard() const -> Bitboard {
    return Bitboard { static_cast<u64>(-static_cast<i64>(raw)) };
  }

  constexpr auto Square::to_bitboard() const -> Bitboard {
    return Bitboard { static_cast<u64>(1) << raw };
  }

}  // namespace rose
