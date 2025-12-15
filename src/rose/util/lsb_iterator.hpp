#pragma once

namespace rose {

  template<class T>
  struct LsbIterator {
    T raw;

    explicit constexpr LsbIterator(T raw) :
        raw(raw) {
    }

    constexpr auto operator++() -> LsbIterator& {
      raw.pop_lsb();
      return *this;
    }

    constexpr auto operator*() const {
      return raw.lsb();
    }

    constexpr auto operator==(const LsbIterator&) const -> bool = default;
  };

}  // namespace rose
