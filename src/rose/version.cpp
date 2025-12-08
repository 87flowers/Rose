#include "rose/version.hpp"

#include <string_view>

namespace rose::version {

  const std::string_view version_string { ROSE_VERSION };
  const std::string_view git_commit_hash { ROSE_GIT_COMMIT_HASH };
  const std::string_view git_commit_desc { ROSE_GIT_COMMIT_DESC };

}  // namespace rose::version
