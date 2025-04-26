#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "rose/util/bch/clmul.h"
#include "rose/util/types.h"

namespace rose::bch {

  template <usize n> using MinPoly = unsigned _BitInt(1 << (n + 1));
  template <usize n> using Generator = unsigned _BitInt(1 << n);

  // represents polynomial of size 2^n
  template <usize n> struct Gf2 {
    static_assert(n >= 3 && n <= 15);

    u32 raw = 0;

    inline static constexpr u32 mask = (1 << n) - 1;
    inline static constexpr u32 primitive_polynomial = [] {
      switch (n) {
      case 3:
        return (1 << 3) + (1 << 1) + 1;
      case 4:
        return (1 << 4) + (1 << 1) + 1;
      case 5:
        return (1 << 5) + (1 << 2) + 1;
      case 6:
        return (1 << 6) + (1 << 1) + 1;
      case 7:
        return (1 << 7) + (1 << 1) + 1;
      case 8:
        return (1 << 8) + (1 << 4) + (1 << 3) + (1 << 2) + 1;
      case 9:
        return (1 << 9) + (1 << 4) + 1;
      case 10:
        return (1 << 10) + (1 << 3) + 1;
      case 11:
        return (1 << 11) + (1 << 2) + 1;
      case 12:
        return (1 << 12) + (1 << 6) + (1 << 4) + (1 << 1) + 1;
      case 13:
        return (1 << 13) + (1 << 4) + (1 << 3) + (1 << 1) + 1;
      case 14:
        return (1 << 14) + (1 << 8) + (1 << 6) + (1 << 1) + 1;
      case 15:
        return (1 << 15) + (1 << 1) + 1;
      }
    }();

    constexpr auto bit(usize i) const -> bool { return (raw >> i) & 1; }
    constexpr auto msb() const -> bool { return bit(n - 1); }
    constexpr auto mul2() const -> Gf2 { return {((raw << 1) ^ (msb() * primitive_polynomial)) & mask}; }

    static constexpr auto add(Gf2 a, Gf2 b) -> Gf2 { return {a.raw ^ b.raw}; }
    static constexpr auto mul(Gf2 a, Gf2 b) -> Gf2 {
      Gf2 result{0};
      for (usize i = 0; i < n; i++) {
        result = result.mul2();
        if (b.bit(n - i - 1))
          result = add(result, a);
      }
      return result;
    }

    constexpr auto pow(usize e) const -> Gf2 {
      Gf2 result{1};
      for (usize i = 0; i < e; i++) {
        result = mul(result, *this);
      }
      return result;
    }

    template <typename P> constexpr auto eval(P poly) const -> Gf2 {
      Gf2 result{0};
      Gf2 term{1};
      for (; poly != 0; poly >>= 1) {
        if (poly & 1)
          result = add(result, term);
        term = mul(term, *this);
      }
      return result;
    }

    constexpr auto findMinPoly() const -> MinPoly<n> {
      for (MinPoly<n> poly = 1;; poly++)
        if (eval(poly).raw == 0)
          return poly;
    }
  };

  template <usize n> inline constexpr auto generateMinPolys() -> std::vector<MinPoly<n>> {
    constexpr Gf2<n> two{2};

    std::vector<MinPoly<n>> result{};
    for (int i = 1; i < (1 << n) - 1; i++) {
      const MinPoly<n> current = two.pow(i).findMinPoly();
      if (!std::ranges::contains(result, current))
        result.push_back(current);
    }

    return result;
  }

  template <usize n> inline constexpr auto generateGenerators(std::vector<MinPoly<n>> min_polys) -> std::vector<Generator<n>> {
    std::vector<Generator<n>> result{};
    Generator<n> current{1};
    for (MinPoly<n> p : min_polys) {
      current = clmul(current, static_cast<Generator<n>>(p));
      result.push_back(current);
    }
    return result;
  }

} // namespace rose::bch
