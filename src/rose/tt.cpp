#include "rose/tt.h"

#include <bit>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <print>

#include "rose/common.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose::tt {

  static constexpr inline auto splitHash(usize bucket_count, u64 hash) -> std::tuple<usize, u8, u64> {
    const u128 mul = static_cast<u128>(hash) * bucket_count;
    const usize index = static_cast<usize>(mul >> 64);
    const u8 ctrl = static_cast<u8>(static_cast<u64>(mul) >> 56);
    const u64 fragment = (static_cast<u64>(mul) >> (56 - Entry::fragment_width)) & Entry::fragment_mask;
    return {index, ctrl, fragment};
  }

  static auto getEntryIndex(const Bucket &bucket, u8 ctrl) -> std::optional<usize> {
    const u16 matches = vec::eq8(bucket.ctrls, v128::broadcast8(ctrl));
    const usize index = std::countr_zero(matches);
    return index < Bucket::entry_count ? std::optional<usize>{index} : std::nullopt;
  }

  auto TT::bucket_alloc(std::size_t bucket_count) -> Bucket * {
    return static_cast<Bucket *>(std::aligned_alloc(4096, bucket_count * sizeof(Bucket)));
  }

  auto TT::bucket_free(Bucket *ptr) -> void { return std::free(ptr); }

  auto TT::clear() -> void { std::memset(buckets.get(), 0, bucket_count * sizeof(Bucket)); }

  auto TT::load(u64 hash, int ply) const -> LookupResult {
    const auto [bucket_index, ctrl, fragment] = splitHash(bucket_count, hash);
    const Bucket &bucket = buckets.get()[bucket_index];

    if (const auto entry_index = getEntryIndex(bucket, ctrl)) {
      const Entry entry = bucket.entries[*entry_index];
      if (entry.fragment() == fragment) {
        return entry.toResult(ply);
      }
    }
    return {};
  }

  auto TT::store(u64 hash, int ply, LookupResult lr) -> void {
    const auto [bucket_index, ctrl, fragment] = splitHash(bucket_count, hash);
    Bucket &bucket = buckets.get()[bucket_index];

    const usize entry_index = getEntryIndex(bucket, ctrl)
                                  .or_else([&] {
                                    const usize res = bucket.ctrls.b.back();
                                    bucket.ctrls.b.back() = (res + 1) % Bucket::entry_count;
                                    return std::optional<usize>{res};
                                  })
                                  .value();

    bucket.ctrls.b[entry_index] = ctrl;
    bucket.entries[entry_index] = Entry{fragment, ply, lr};
  }

  auto TT::print(u64 hash) const -> void {
    const auto [bucket_index, ctrl, fragment] = splitHash(bucket_count, hash);
    std::print("hash:   {:016x}\n", hash);
    std::print("ctrl:   {:02x}\n", ctrl);
    std::print("frag:   {:06x}\n", fragment);
    std::print("bucket index: 0x{:x}/0x{:x}\n", bucket_index, bucket_count);

    const Bucket &bucket = buckets.get()[bucket_index];
    std::print("bucket ctrls:");
    for (usize i = 0; i < Bucket::entry_count; i++)
      std::print(" {:02x}", bucket.ctrls.b[i]);
    std::print("\n");

    if (const auto entry_index = getEntryIndex(bucket, ctrl)) {
      std::print("entry index: {}\n", *entry_index);
      const Entry entry = bucket.entries[*entry_index];
      std::print("entry raw:   {:016x}\n", entry.raw);
      if (entry.fragment() != fragment)
        std::print("entry frag:  {:06x} [mismatch!]\n", entry.fragment());
      std::print("entry depth: {}\n", entry.depth());
      std::print("entry score: {}\n", entry.score(0));
      std::print("entry bound: {}\n", [&]() -> const char * {
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
      std::print("entry move:  {}\n", entry.move());
    } else {
      std::print("no matching ctrl found.\n");
    }
  }

} // namespace rose::tt
