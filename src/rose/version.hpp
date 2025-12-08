#pragma once

#include <string_view>

namespace rose::version {

  extern const std::string_view version_string;
  extern const std::string_view git_commit_hash;
  extern const std::string_view git_commit_desc;

}  // namespace rose::version
