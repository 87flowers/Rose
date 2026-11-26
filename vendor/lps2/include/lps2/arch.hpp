#pragma once

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)

#define LPS2_ARCH_X86_64 1

#if defined(__AVX512VBMI2__)
#define LPS2_ARCH_AVX512VBMI2 1
#endif

#if defined(__AVX512VBMI__)
#define LPS2_ARCH_AVX512VBMI 1
#endif

#if defined(__AVX512F__) && defined(__AVX512VL__) && defined(__AVX512DQ__) && defined(__AVX512BW__)
#define LPS2_ARCH_AVX512BASE 1
#endif

#if defined(__AVX512VNNI__)
#define LPS2_ARCH_AVX512VNNI 1
#endif

#if defined(__AVX__)
#define LPS2_ARCH_AVX 1
#endif

#if defined(__AVX2__)
#define LPS2_ARCH_AVX2 1
#endif

#if defined(__AVX_VNNI__)
#define LPS2_ARCH_AVX_VNNI 1
#endif

#if defined(__SSE4_2__)
#define LPS2_ARCH_SSE4_2 1
#endif

#if defined(__SSE4_1__)
#define LPS2_ARCH_SSE4_1 1
#endif

#if defined(__SSSE3__)
#define LPS2_ARCH_SSSE3 1
#endif

#endif

#if defined(__aarch64__) || defined(_M_ARM64)

#define LPS2_ARCH_AARCH64 1

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define LPS2_ARCH_NEON 1
#endif

#endif

#if LPS2_ARCH_AVX512BASE
#define LPS2_PREFERS_BIT_MASK 1
#else
#define LPS2_PREFERS_VECTOR_MASK 1
#endif
