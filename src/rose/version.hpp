#pragma once

#include <fmt/format.h>
#include <string_view>

namespace rose::version {

  extern const std::string_view version_string;
  extern const std::string_view git_commit_hash;
  extern const std::string_view git_commit_desc;

  inline auto to_string() -> std::string {
    return fmt::format("{}", rose::version::version_string);
  }

}  // namespace rose::version
