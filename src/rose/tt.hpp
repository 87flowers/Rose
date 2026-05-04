#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/score.hpp"

#include <memory>
#include <optional>

namespace rose::tt {

  inline constexpr usize default_hash_size_mb = 64;
  inline constexpr usize maximum_hash_size_mb = 1048576;  // semi-arbitrary

  enum class Bound {
    none = 0b00,
    lower_bound = 0b01,
    exact = 0b10,
    upper_bound = 0b11,
  };

  struct LookupResult {
    i32 depth = 0;
    Bound bound = Bound::none;
    Score score = 0;
    Move move = Move::none();
  };

  struct Entry {
    // MSB -> LSB
    // i16 score
    // u16 move
    // u8 depth
    // u2 bounds
    // u22 fragment
    static inline constexpr usize fragment_width = 22;
    static inline constexpr usize bounds_shift = 22;
    static inline constexpr usize depth_shift = 24;
    static inline constexpr usize move_shift = 32;
    static inline constexpr usize score_shift = 48;
    static inline constexpr u64 fragment_mask = (static_cast<u64>(1) << fragment_width) - 1;

    u64 raw = 0;

    constexpr Entry(u64 fragment, i32 ply, LookupResult lr) {
      const Score tt_score = score::adjust_plys_to_mate(lr.score, -ply);
      const i32 tt_depth = std::clamp(lr.depth, 0, 255);
      const u64 tt_bound = std::to_underlying(lr.bound);

      rose_assert((fragment & fragment_mask) == fragment);

      raw = 0;
      raw |= fragment;
      raw |= static_cast<u64>(tt_bound) << bounds_shift;
      raw |= static_cast<u64>(tt_depth) << depth_shift;
      raw |= static_cast<u64>(lr.move.raw) << move_shift;
      raw |= static_cast<u64>(tt_score) << score_shift;
    }

    constexpr inline auto fragment() const -> u64 {
      return raw & fragment_mask;
    }

    constexpr inline auto bound() const -> Bound {
      return static_cast<Bound>((raw >> bounds_shift) & 3);
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
        .move = move(),
      };
    }
  };

  static_assert(sizeof(Entry) == sizeof(u64));

  constexpr inline auto megabytes_to_entry_count(usize mb) -> usize {
    return mb * 1024 * 1024 / sizeof(Entry);
  }

  struct TT {
  private:
    static auto tt_alloc(usize mb) -> Entry*;
    static auto tt_free(Entry* ptr) -> void;

    usize m_count;
    std::unique_ptr<Entry, decltype(&tt_free)> m_table;

  public:
    explicit TT(usize mb) :
        m_count {megabytes_to_entry_count(mb)},
        m_table {tt_alloc(mb), &tt_free} {
    }

    auto resize(usize mb) -> void {
      m_count = megabytes_to_entry_count(mb);
      m_table = {tt_alloc(mb), &tt_free};
    }

    auto clear() -> void;

    auto load(u64 hash, int ply) const -> std::optional<LookupResult>;
    auto store(u64 hash, int ply, LookupResult lr) -> void;

    auto print(u64 hash) const -> void;
  };

}  // namespace rose::tt
