#pragma once

#include <optional>
#include <string_view>

namespace rose {

  struct Tokenizer {
  private:
    std::string_view str;

    inline static constexpr std::string_view whitespace{" \t\r\n\f\v"};

    constexpr auto trimStart() -> void {
      const usize index = str.find_first_not_of(whitespace);
      if (index != std::string_view::npos)
        str.remove_prefix(index);
    }

    constexpr auto getNext() -> std::string_view {
      const usize index = str.find_first_of(whitespace);
      const std::string_view result = str.substr(0, index);
      str.remove_prefix(index != std::string_view::npos ? index : str.size());
      return result;
    }

  public:
    explicit constexpr Tokenizer(std::string_view str) : str(str) { trimStart(); }

    inline constexpr auto rest() const -> std::string_view { return str; }

    inline constexpr auto next() -> std::string_view {
      const std::string_view result = getNext();
      trimStart();
      return result;
    }
  };

} // namespace rose
