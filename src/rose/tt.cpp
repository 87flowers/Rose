#include "rose/tt.hpp"

#include "rose/common.hpp"
#include "rose/node_type.hpp"

#include <cstdlib>
#include <cstring>
#include <fmt/format.h>

namespace rose::tt {

#ifdef _WIN32

  auto TT::table_alloc(std::size_t m_count) -> Bucket* {
    return static_cast<Bucket*>(_aligned_malloc(m_count * sizeof(Bucket), 4096));
  }

  auto TT::table_free(Bucket* ptr) -> void {
    return _aligned_free(ptr);
  }

#else

  auto TT::table_alloc(std::size_t m_count) -> Bucket* {
    return static_cast<Bucket*>(std::aligned_alloc(4096, m_count * sizeof(Bucket)));
  }

  auto TT::table_free(Bucket* ptr) -> void {
    return std::free(ptr);
  }

#endif

  auto TT::clear() -> void {
    std::memset(m_table.get(), 0, m_count * sizeof(Bucket));
  }

  auto TT::load(Hash hash, int ply) const -> LookupResult {
    const auto [index, fragment] = split_hash(m_count, hash);
    const Bucket& bucket = m_table.get()[index];

    if (const usize j = bucket.lookup_fragment(fragment); j < bucket.entries.size()) {
      const Entry& entry = bucket.entries[j];
      return entry.to_result(ply);
    }
    return {};
  }

  auto TT::store(Hash hash, int ply, LookupResult lr) -> void {
    const auto retention_score = [this](const Entry& e) {
      constexpr int max_age = Entry::age_mask + 1;
      return e.depth() - (max_age + m_age - e.age()) % max_age * 4;
    };

    const auto [index, fragment] = split_hash(m_count, hash);
    Bucket& bucket = m_table.get()[index];

    if (const usize j = bucket.lookup_fragment(fragment); j < bucket.entries.size()) {
      Entry& entry = bucket.entries[j];

      if (lr.move.is_none())
        lr.move = entry.move();

      if ((entry.bound() != NodeType::pv && lr.bound == NodeType::pv && lr.depth > 0) || (lr.depth * 2 >= retention_score(entry)))
        entry = Entry {ply, lr, m_age};

      return;
    }

    Entry best_entry = bucket.entries[0];
    usize best_index = 0;

    if (best_entry.bound() != NodeType::none) {
      for (usize j = 1; j < bucket.entries.size(); j++) {
        Entry& entry = bucket.entries[j];
        if (entry.bound() == NodeType::none) {
          best_index = j;
          break;
        }
        if (retention_score(entry) < retention_score(best_entry)) {
          best_entry = entry;
          best_index = j;
        }
      }
    }

    bucket.entries[best_index] = Entry {ply, lr, m_age};
    bucket.set_fragment(best_index, fragment);
  }

  auto TT::print(Hash hash) const -> void {
    const auto [index, fragment] = split_hash(m_count, hash);
    fmt::print("hash:   {:016x}\n", hash);
    fmt::print("frag:   {:06x}\n", fragment);
    fmt::print("index:  0x{:x}/0x{:x}\n", index, m_count);

    const Bucket& bucket = m_table.get()[index];

    if (const usize j = bucket.lookup_fragment(fragment); j < bucket.entries.size()) {
      const Entry& entry = bucket.entries[j];
      fmt::print("entry raw:   {:016x}\n", entry.raw);
      fmt::print("entry age:   {}\n", entry.age());
      fmt::print("entry depth: {}\n", entry.depth());
      fmt::print("entry score: {}\n", entry.score(0));
      fmt::print("entry bound: {}\n", entry.bound());
      fmt::print("entry move:  {}\n", entry.move().to_string(MoveFormat::frc));
    } else {
      fmt::print("not found in bucket\n");
    }
  }

}  // namespace rose::tt
