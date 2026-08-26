#pragma once

#include "rose/common.hpp"
#include "rose/hash.hpp"
#include "rose/move.hpp"
#include "rose/node_type.hpp"
#include "rose/score.hpp"

#include <bit>
#include <memory>
#include <tuple>

namespace rose::tt {

  inline constexpr usize default_hash_size_mb = 64;
  inline constexpr usize maximum_hash_size_mb = 1048576;

  struct LookupResult {
    i32 depth = 0;
    NodeType bound = NodeType::none;
    Score score = score::none;
    Score raw_eval = score::none;
    Move move = Move::none();

    auto is_none() const -> bool {
      return bound == NodeType::none;
    }

    auto is_some() const -> bool {
      return bound != NodeType::none;
    }
  };

  struct Entry {
    // MSB -> LSB
    // i16 score
    // u16 move
    // u8 depth
    // u2 bounds
    // u5 age
    // u1 unused
    // i16 raw_eval
    static inline constexpr usize raw_eval_shift = 0;
    static inline constexpr usize age_shift = 17;
    static inline constexpr usize bounds_shift = 22;
    static inline constexpr usize depth_shift = 24;
    static inline constexpr usize move_shift = 32;
    static inline constexpr usize score_shift = 48;

    static inline constexpr usize age_width = 5;
    static inline constexpr int age_mask = (1 << age_width) - 1;

    u64 raw = 0;

    constexpr Entry(i32 ply, LookupResult lr, int age) {
      const i32 tt_score = score::adjust_plys_to_mate(lr.score, -ply);
      const i32 tt_depth = std::clamp(lr.depth, 0, 255);
      const u64 tt_bound = std::to_underlying(lr.bound.raw);
      const u64 tt_raw_eval = static_cast<u64>(lr.raw_eval) & 0xFFFF;

      raw = 0;
      raw |= static_cast<u64>(tt_raw_eval) << raw_eval_shift;
      raw |= static_cast<u64>(age & age_mask) << age_shift;
      raw |= static_cast<u64>(tt_bound) << bounds_shift;
      raw |= static_cast<u64>(tt_depth) << depth_shift;
      raw |= static_cast<u64>(lr.move.raw) << move_shift;
      raw |= static_cast<u64>(tt_score) << score_shift;
    }

    constexpr inline auto raw_eval() const -> Score {
      const usize sext_shift = 64 - 16;
      return static_cast<Score>(static_cast<i64>(raw >> raw_eval_shift << sext_shift) >> sext_shift);
    }

    constexpr inline auto age() const -> int {
      return static_cast<int>((raw >> age_shift) & age_mask);
    }

    constexpr inline auto bound() const -> NodeType {
      return static_cast<NodeType::Underlying>((raw >> bounds_shift) & 3);
    }

    constexpr inline auto depth() const -> u8 {
      return static_cast<u8>(raw >> depth_shift);
    }

    constexpr inline auto move() const -> Move {
      return Move {static_cast<u16>(raw >> move_shift)};
    }

    constexpr inline auto score(i32 ply) const -> Score {
      const Score tt_score = static_cast<Score>(static_cast<i64>(raw) >> score_shift);
      return score::adjust_plys_to_mate(tt_score, +ply);
    }

    constexpr auto to_result(i32 ply) const -> LookupResult {
      return {
        .depth = depth(),
        .bound = bound(),
        .score = score(ply),
        .raw_eval = raw_eval(),
        .move = move(),
      };
    }
  };

  static_assert(sizeof(Entry) == sizeof(u64));

  struct Bucket {
    static inline constexpr usize entry_count = 3;
    static inline constexpr usize fragment_width = 21;
    static inline constexpr u64 fragment_mask = (u64 {1} << fragment_width) - 1;
    static_assert(entry_count * fragment_width < 64);

    std::array<Entry, entry_count> entries;
    u64 fragments;

    auto fragment(usize index) const -> u64 {
      const usize shift = index * fragment_width;
      return (fragments >> shift) & fragment_mask;
    }

    auto set_fragment(usize index, u64 fragment) -> void {
      const usize shift = index * fragment_width;
      fragments &= ~(fragment_mask << shift);
      fragments |= fragment << shift;
    }

    auto lookup_fragment(u64 fragment) const -> usize {
      constexpr u64 bits = u64 {1} | (u64 {1} << fragment_width) | (u64 {1} << (fragment_width * 2)) | (u64 {1} << (fragment_width * 3));
      const u64 needle = bits * fragment;
      const u64 zeros = fragments ^ needle;
      const u64 matches = (zeros - bits) & ~zeros & (bits << (fragment_width - 1));
      return static_cast<usize>(std::countr_zero(matches) / fragment_width);
    }
  };

  static_assert(sizeof(Bucket) == sizeof(u64) * 4);

  constexpr inline auto mb_to_count(usize mb) -> usize {
    return mb * 1024 * 1024 / sizeof(Bucket);
  }

  constexpr inline auto split_hash(usize count, Hash hash) -> std::tuple<usize, u64> {
    const u128 mul = static_cast<u128>(hash) * count;
    const usize index = static_cast<usize>(mul >> 64);
    const u64 fragment = (static_cast<u64>(mul) >> (64 - Bucket::fragment_width)) & Bucket::fragment_mask;
    return {index, fragment};
  }

  struct TT {
  private:
    static auto table_alloc(std::size_t m_count) -> Bucket*;
    static auto table_free(Bucket* ptr) -> void;

    int m_age = 0;
    usize m_count;
    std::unique_ptr<Bucket, decltype(&table_free)> m_table;

  public:
    explicit TT(usize mb) :
        m_count {mb_to_count(mb)},
        m_table {table_alloc(m_count), &table_free} {
      clear();
    }

    auto resize(usize mb) -> void {
      m_count = mb_to_count(mb);
      m_table = {table_alloc(m_count), &table_free};
      clear();
    }

    auto clear() -> void;

    auto increment_age() -> void {
      m_age = (m_age + 1) & Entry::age_mask;
    }

    auto prefetch(Hash hash) -> void {
      const auto [index, fragment] = split_hash(m_count, hash);
      const Bucket& bucket = m_table.get()[index];
      __builtin_prefetch(&bucket, 0, 2);
    }

    auto load(Hash hash, int ply) const -> LookupResult;
    auto store(Hash hash, int ply, LookupResult lr) -> void;

    auto print(Hash hash) const -> void;
  };

}  // namespace rose::tt
