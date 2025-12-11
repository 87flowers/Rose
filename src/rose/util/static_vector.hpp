#pragma once

#include "rose/common.hpp"
#include "rose/util/assert.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace rose {

  template<typename T, usize cap>
  class StaticVector {
  public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
    using size_type = usize;

    constexpr StaticVector() = default;
    ~StaticVector() = default;

    constexpr StaticVector(const StaticVector& other) :
        len(other.len) {
      std::copy(other.begin(), other.end(), begin());
    }

    constexpr StaticVector(StaticVector&& other) :
        len(std::exchange(other.len, 0)) {
      std::move(other.begin(), other.end(), begin());
    }

    constexpr StaticVector& operator=(const StaticVector& other) {
      len = other.len;
      std::copy(other.begin(), other.end(), begin());
      return *this;
    }

    constexpr StaticVector& operator=(StaticVector&& other) {
      len = std::exchange(other.len, 0);
      std::move(other.begin(), other.end(), begin());
      return *this;
    }

    constexpr auto push_back(const T& value) -> iterator {
      rose_assert(len < cap);
      data[len] = value;
      return &data[len++];
    }

    constexpr auto append(StaticVector&& other) -> void {
      rose_assert(len + other.len < cap);
      std::move(other.begin(), other.end(), end());
      len += std::exchange(other.len, 0);
    }

    constexpr auto clear() -> void {
      len = 0;
    }

    constexpr auto size() const -> usize {
      return len;
    }

    constexpr auto capacity() const -> usize {
      return cap;
    }

    constexpr auto empty() const -> bool {
      return len == 0;
    }

    constexpr auto resize(usize new_size) -> void {
      rose_assert(new_size <= cap);
      len = new_size;
    }

    constexpr auto operator[](usize index) -> T& {
      rose_assert(index < len);
      return data[index];
    }

    constexpr auto operator[](usize index) const -> const T& {
      rose_assert(index < len);
      return data[index];
    }

    constexpr auto begin() -> iterator {
      return &data[0];
    }

    constexpr auto begin() const -> const_iterator {
      return &data[0];
    }

    constexpr auto cbegin() const -> const_iterator {
      return begin();
    }

    constexpr auto end() -> iterator {
      return &data[len];
    }

    constexpr auto end() const -> const_iterator {
      return &data[len];
    }

    constexpr auto cend() const -> const_iterator {
      return end();
    }
  protected:
    usize len = 0;
    std::array<T, cap> data;
  };

}  // namespace rose
