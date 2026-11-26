#pragma once

#include "lps2/arch.hpp"

// clang-format off
#include "lps2/vector_ops/zero.hpp"
#include "lps2/vector_ops/splat.hpp"
#include "lps2/vector_ops/load.hpp"
#include "lps2/vector_ops/store.hpp"
#include "lps2/vector_ops/read.hpp"
#include "lps2/vector_ops/convert.hpp"
#include "lps2/vector_ops/extract_aligned.hpp"
#include "lps2/vector_ops/swizzle_vector.hpp"
#include "lps2/vector_ops/swizzle_double_vector.hpp"
#include "lps2/vector_ops/swizzle_128bit.hpp"
#include "lps2/vector_ops/swizzle_vm.hpp"
#include "lps2/vector_ops/swizzle_bm.hpp"
#include "lps2/vector_ops/shl.hpp"
#include "lps2/vector_ops/shr.hpp"
#include "lps2/vector_ops/andnot.hpp"
#include "lps2/vector_ops/clamp.hpp"
#include "lps2/vector_ops/pair_dot.hpp"
#include "lps2/vector_ops/accumulate_pair_dot.hpp"
#include "lps2/vector_ops/reduce_add.hpp"
#include "lps2/vector_ops/reduce_or.hpp"
#include "lps2/vector_ops/reduce_xor.hpp"
#include "lps2/vector_ops/zip_low.hpp"
#include "lps2/vector_ops/zip_high.hpp"
#include "lps2/vector_ops/zip_low_128bit.hpp"
#include "lps2/vector_ops/zip_high_128bit.hpp"
#include "lps2/vector_ops/operator_eq.hpp"
#include "lps2/vector_ops/operator_not.hpp"
#include "lps2/vector_ops/operator_and.hpp"
#include "lps2/vector_ops/operator_or.hpp"
#include "lps2/vector_ops/operator_xor.hpp"
#include "lps2/vector_ops/operator_add.hpp"
#include "lps2/vector_ops/operator_sub.hpp"
#include "lps2/vector_ops/operator_mul.hpp"
#include "lps2/vector_ops/operator_shl.hpp"
#include "lps2/vector_ops/operator_shr.hpp"
// clang-format on

#if LPS2_PREFERS_VECTOR_MASK
// clang-format off
#include "lps2/vector_ops/test_vm.hpp"
#include "lps2/vector_ops/eq_vm.hpp"
#include "lps2/vector_ops/neq_vm.hpp"
#include "lps2/vector_ops/gt_vm.hpp"
#include "lps2/vector_ops/nonzeros_vm.hpp"
#include "lps2/vector_ops/zeros_vm.hpp"
#include "lps2/vector_ops/msb_vm.hpp"
// clang-format on
#endif

#if LPS2_PREFERS_BIT_MASK
// clang-format off
#include "lps2/vector_ops/test_bm.hpp"
#include "lps2/vector_ops/eq_bm.hpp"
#include "lps2/vector_ops/neq_bm.hpp"
#include "lps2/vector_ops/gt_bm.hpp"
#include "lps2/vector_ops/nonzeros_bm.hpp"
#include "lps2/vector_ops/zeros_bm.hpp"
#include "lps2/vector_ops/msb_bm.hpp"
// clang-format on
#endif
