#pragma once

#include <array>
#include <optional>
#include <tuple>

#include "rose/depth.h"
#include "rose/eval/eval.h"
#include "rose/move.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose::tt {

  inline constexpr usize default_hash_size_mb = 64;

  enum class Bound {
    none = 0b00,
    lower_bound = 0b01,
    exact = 0b10,
    upper_bound = 0b11,
  };

  struct LookupResult {
    Depth depth = Depth::zero();
    Bound bound = Bound::none;
    i32 score = 0;
    Move move = Move::none();
  };

  struct Entry {
    // MSB -> LSB
    // i16 score
    // u10 compressed_move
    // u14 depth
    // u2 bounds
    // u22 fragment
    static inline constexpr usize fragment_width = 22;
    static inline constexpr usize bounds_shift = 22;
    static inline constexpr usize depth_shift = 24;
    static inline constexpr usize move_shift = 38;
    static inline constexpr usize score_shift = 48;
    static inline constexpr u64 fragment_mask = (static_cast<u64>(1) << fragment_width) - 1;

    u64 raw = 0;

    constexpr Entry(u64 fragment, i32 ply, LookupResult lr, const Position &context) {
      const i32 tt_score = eval::adjustPlysToMate(lr.score, -ply);
      const i32 tt_depth = std::clamp(lr.depth.raw, 0, 256 * Depth::granularity - 1);
      const u64 tt_bound = std::to_underlying(lr.bound);

      rose_assert((fragment & fragment_mask) == fragment);

      raw = 0;
      raw |= fragment;
      raw |= static_cast<u64>(tt_bound) << bounds_shift;
      raw |= static_cast<u64>(tt_depth) << depth_shift;
      raw |= static_cast<u64>(lr.move.compress(context)) << move_shift;
      raw |= static_cast<u64>(tt_score) << score_shift;
    }

    constexpr inline auto fragment() const -> u64 { return raw & fragment_mask; }
    constexpr inline auto bound() const -> Bound { return static_cast<Bound>((raw >> bounds_shift) & 3); }
    constexpr inline auto depth() const -> Depth { return {static_cast<i32>((raw >> depth_shift) & 0x3FFF)}; }
    constexpr inline auto move(const Position &context) const -> Move { return Move::decompress((raw >> move_shift) & 0x3FF, context); }
    constexpr inline auto score(i32 ply) const -> i32 {
      const i32 tt_score = static_cast<i32>(static_cast<i64>(raw) >> score_shift);
      return eval::adjustPlysToMate(tt_score, +ply);
    }

    constexpr auto toResult(i32 ply, const Position &context) const -> LookupResult {
      return {
          .depth = depth(),
          .bound = bound(),
          .score = score(ply),
          .move = move(context),
      };
    }
  };
  static_assert(sizeof(Entry) == sizeof(u64));

  struct Bucket {
    static inline constexpr usize entry_count = 14;
    v128 ctrls;
    std::array<Entry, entry_count> entries;
  };
  static_assert(sizeof(Bucket) == sizeof(u64) * 16);

  constexpr inline auto megabytesToBucketCount(usize mb) -> usize { return mb * 1024 * 1024 / sizeof(Bucket); }

  struct TT {
  private:
    static auto bucket_alloc(std::size_t bucket_count) -> Bucket *;
    static auto bucket_free(Bucket *ptr) -> void;

    usize bucket_count;
    std::unique_ptr<Bucket, decltype(&bucket_free)> buckets;

  public:
    explicit TT(usize mb) : bucket_count{megabytesToBucketCount(mb)}, buckets{bucket_alloc(bucket_count), &bucket_free} {}
    auto resize(usize mb) -> void {
      bucket_count = megabytesToBucketCount(mb);
      buckets = {bucket_alloc(bucket_count), &bucket_free};
    }

    auto clear() -> void;

    auto load(u64 hash, int ply, const Position &context) const -> LookupResult;
    auto store(u64 hash, int ply, LookupResult lr, const Position &context) -> void;
  };

} // namespace rose::tt
