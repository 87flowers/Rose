#include "rose/util/string.h"

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "rose/util/types.h"

namespace rose {

  auto stringSplit(std::string text, char delim) -> std::vector<std::string> {
    std::string line;
    std::vector<std::string> v;
    std::stringstream ss(text);
    while (std::getline(ss, line, delim))
      v.push_back(line);
    return v;
  }

  template <typename Int> auto parseInteger(std::string_view str) -> std::optional<Int> {
    bool negate = false;
    Int result = 0;
    usize i = 0;

    if (str.size() == 0)
      return std::nullopt;

    if (str[0] == '-') {
      if (!std::is_signed_v<Int> || str.size() == 1)
        return std::nullopt;
      negate = true;
      i = 1;
    }

    for (; i < str.size(); i++) {
      if (str[i] < '0' && str[i] > '9')
        return std::nullopt;
      if (__builtin_mul_overflow(result, static_cast<Int>(10), &result))
        return std::nullopt;
      if (__builtin_add_overflow(result, static_cast<Int>(str[i] - '0'), &result))
        return std::nullopt;
    }

    if constexpr (std::is_signed_v<Int>) {
      return negate ? -result : result;
    } else {
      return result;
    }
  }

  auto parseInt(std::string_view str) -> std::optional<int> { return parseInteger<int>(str); }
  auto parseU32(std::string_view str) -> std::optional<u32> { return parseInteger<u32>(str); }
  auto parseU64(std::string_view str) -> std::optional<u64> { return parseInteger<u64>(str); }

} // namespace rose
