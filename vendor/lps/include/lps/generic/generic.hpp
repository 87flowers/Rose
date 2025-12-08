#pragma once

#include "lps/generic/basic_vector_mask.hpp"
#include "lps/generic/generic.fwd.hpp"
#include "lps/generic/vector.hpp"
#include "lps/stdint.hpp"

namespace lps::generic {

  struct environment {
    template<class T, usize N>
    using vector = generic::vector<T, N>;

    template<class T, usize N>
    using vector_mask = generic::vector_mask<T, N>;

    template<class T, usize N>
    using mask = generic::vector_mask<T, N>;
  };

}  // namespace lps::generic
