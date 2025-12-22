#pragma once

#include "rose/common.hpp"
#include "rose/square.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/lsb_iterator.hpp"

#include <bit>

namespace rose {

  struct Bitboard {
    u64 raw = 0;

    using Iterator = LsbIterator<Bitboard>;

    constexpr Bitboard() = default;

    constexpr explicit Bitboard(u64 raw) :
        raw(raw) {
    }

    static constexpr Bitboard file_mask(i8 file) {
      rose_assert(file >= 0 && file <= 7);
      return Bitboard {static_cast<u64>(0x0101010101010101) << file};
    }

    constexpr auto is_empty() const -> bool {
      return raw == 0;
    }

    constexpr auto lsb() const -> Square {
      return Square {static_cast<u8>(std::countr_zero(raw))};
    }

    constexpr auto pop_lsb() -> void {
      raw &= raw - 1;
    }

    constexpr auto begin() const -> Iterator {
      return Iterator {*this};
    }

    constexpr auto end() const -> Iterator {
      return Iterator {Bitboard {}};
    }

    auto read(Square sq) const -> bool {
      return (raw >> sq.raw) & 1;
    }

    friend constexpr auto operator~(Bitboard a) -> Bitboard {
      return Bitboard {~a.raw};
    }

    friend constexpr auto operator&(Bitboard a, Bitboard b) -> Bitboard {
      return Bitboard {a.raw & b.raw};
    }

    friend constexpr auto operator&=(Bitboard& a, Bitboard b) -> Bitboard {
      return a = a & b;
    }

    friend constexpr auto operator|(Bitboard a, Bitboard b) -> Bitboard {
      return Bitboard {a.raw | b.raw};
    }

    friend constexpr auto operator|=(Bitboard& a, Bitboard b) -> Bitboard {
      return a = a | b;
    }

    friend constexpr auto operator^(Bitboard a, Bitboard b) -> Bitboard {
      return Bitboard {a.raw ^ b.raw};
    }

    friend constexpr auto operator^=(Bitboard& a, Bitboard b) -> Bitboard {
      return a = a ^ b;
    }

    friend constexpr auto operator>>(Bitboard a, int shift) -> Bitboard {
      return Bitboard {a.raw >> shift};
    }

    friend constexpr auto operator<<(Bitboard a, int shift) -> Bitboard {
      return Bitboard {a.raw << shift};
    }

    constexpr auto operator==(const Bitboard&) const -> bool = default;
  };

  constexpr auto Color::to_bitboard() const -> Bitboard {
    return Bitboard {static_cast<u64>(-static_cast<i64>(raw))};
  }

  constexpr auto Square::to_bitboard() const -> Bitboard {
    return Bitboard {static_cast<u64>(1) << raw};
  }

}  // namespace rose
