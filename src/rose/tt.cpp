#include "rose/tt.hpp"

#include "rose/common.hpp"

#include <cstdlib>
#include <cstring>
#include <fmt/format.h>

namespace rose::tt {

  auto TT::table_alloc(std::size_t m_count) -> Entry* {
    return static_cast<Entry*>(std::aligned_alloc(4096, m_count * sizeof(Entry)));
  }

  auto TT::table_free(Entry* ptr) -> void {
    return std::free(ptr);
  }

  auto TT::clear() -> void {
    std::memset(m_table.get(), 0, m_count * sizeof(Entry));
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
    fmt::print("entry bound: {}\n", entry.bound());
    fmt::print("entry move:  {}\n", entry.move().to_string(MoveFormat::frc));
  }

}  // namespace rose::tt
