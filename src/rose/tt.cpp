#include "rose/tt.hpp"

#include "rose/common.hpp"

#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <tuple>

namespace rose::tt {

  static constexpr inline auto split_hash(usize count, u64 hash) -> std::tuple<usize, u64> {
    const u128 mul = static_cast<u128>(hash) * count;
    const usize index = static_cast<usize>(mul >> 64);
    const u64 fragment = (static_cast<u64>(mul) >> (64 - Entry::fragment_width)) & Entry::fragment_mask;
    return {index, fragment};
  }

  auto TT::table_alloc(std::size_t m_count) -> Entry* {
    return static_cast<Entry*>(std::aligned_alloc(4096, m_count * sizeof(Entry)));
  }

  auto TT::table_free(Entry* ptr) -> void {
    return std::free(ptr);
  }

  auto TT::clear() -> void {
    std::memset(m_table.get(), 0, m_count * sizeof(Entry));
  }

  auto TT::load(u64 hash, int ply) const -> LookupResult {
    const auto [index, fragment] = split_hash(m_count, hash);
    const Entry& entry = m_table.get()[index];

    if (entry.fragment() == fragment) {
      return entry.toResult(ply);
    }
    return {};
  }

  auto TT::store(u64 hash, int ply, LookupResult lr) -> void {
    const auto [index, fragment] = split_hash(m_count, hash);
    Entry& entry = m_table.get()[index];

    entry = Entry {fragment, ply, lr};
  }

  auto TT::print(u64 hash) const -> void {
    const auto [index, fragment] = split_hash(m_count, hash);
    fmt::print("hash:   {:016x}\n", hash);
    fmt::print("frag:   {:06x}\n", fragment);
    fmt::print("index:  0x{:x}/0x{:x}\n", index, m_count);

    const Entry& entry = m_table.get()[index];
    fmt::print("entry raw:   {:016x}\n", entry.raw);
    fmt::print("entry frag:  {:06x}{}\n", entry.fragment(), entry.fragment() != fragment ? " [mismatch!]" : "");
    fmt::print("entry depth: {}\n", entry.depth());
    fmt::print("entry score: {}\n", entry.score(0));
    fmt::print("entry bound: {}\n", [&]() -> const char* {
      switch (entry.bound()) {
      case Bound::none:
        return "none";
      case Bound::lower_bound:
        return "lower_bound";
      case Bound::exact:
        return "exact";
      case Bound::upper_bound:
        return "upper_bound";
      default:
        return "unknown";
      }
    }());
    fmt::print("entry move:  {}\n", entry.move().to_string(MoveFormat::frc));
  }

}  // namespace rose::tt
