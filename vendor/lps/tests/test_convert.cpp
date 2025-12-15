#include "tests.hpp"

#include <bit>
#include <cstdint>
#include <lps/generic/vector.hpp>
#include <lps/lps.hpp>

using namespace lps;
using namespace lps::prelude;

constexpr std::array<u8, 16> bytes16 {
  0xc8, 0xff, 0x9f, 0xb5, 0xab, 0x9f, 0xcf, 0x00, 0x3e, 0x89, 0x84, 0xd5, 0x59, 0x2a, 0xe0, 0x47,  //
};
constexpr std::array<u8, 32> bytes32 {
  0x2b, 0x53, 0x17, 0x22, 0x97, 0xe8, 0xa8, 0x62, 0x86, 0x76, 0xd9, 0x81, 0x4d, 0xc6, 0x2f, 0xd5,  //
  0x09, 0xc8, 0xcb, 0xdb, 0x42, 0x31, 0xab, 0x0f, 0x30, 0xf9, 0x8d, 0x9c, 0xe0, 0xc6, 0xe1, 0x45,  //
};
constexpr std::array<u8, 64> bytes64 {
  0x38, 0xe4, 0x43, 0xb5, 0xec, 0xf1, 0xd5, 0xcc, 0xc1, 0x19, 0x01, 0x98, 0x6d, 0xe0, 0x27, 0x9f,  //
  0xd3, 0x49, 0xc4, 0xaa, 0x3c, 0x41, 0x6a, 0xb8, 0x8b, 0xf6, 0x29, 0xe6, 0xff, 0xe8, 0x16, 0x67,  //
  0x03, 0x59, 0xed, 0x5f, 0xa5, 0xec, 0x01, 0xb3, 0xcb, 0x14, 0xf0, 0xb3, 0x12, 0xac, 0x1e, 0x84,  //
  0xd3, 0x6a, 0x7a, 0xfc, 0xbd, 0xb5, 0xa5, 0xfd, 0x75, 0x55, 0x78, 0x22, 0x6e, 0xbb, 0x13, 0xe7,  //
};
constexpr std::array<u8, 128> bytes128 {
  0xdc, 0x79, 0x21, 0x49, 0xb8, 0xc6, 0xf3, 0xc5, 0xee, 0xe0, 0x02, 0x2c, 0xe7, 0xf7, 0x4f, 0x2e,  //
  0xc7, 0x68, 0x41, 0x54, 0x4d, 0xc2, 0x0c, 0x4a, 0x3d, 0xe7, 0xb4, 0x23, 0x03, 0x67, 0xe6, 0x21,  //
  0x64, 0x87, 0x9b, 0xa2, 0x4c, 0x5d, 0x8b, 0x30, 0xb8, 0x59, 0xb7, 0x47, 0xbd, 0x8e, 0x9e, 0x31,  //
  0x4e, 0x0c, 0xf1, 0xf6, 0xb2, 0xf8, 0x36, 0xe3, 0xbe, 0x62, 0x69, 0x35, 0xbb, 0xed, 0x22, 0xe5,  //
  0xf7, 0xc3, 0x42, 0xdb, 0xd0, 0x60, 0xd2, 0xd1, 0x66, 0x9c, 0x7f, 0xff, 0x2c, 0x35, 0xd7, 0xef,  //
  0xb0, 0x0a, 0xc2, 0x1e, 0x00, 0x0a, 0xe8, 0x49, 0x62, 0xc7, 0x87, 0x47, 0xbd, 0x02, 0x44, 0x46,  //
  0xff, 0x21, 0x9b, 0x69, 0x1e, 0x03, 0xdf, 0x64, 0x90, 0x05, 0x2c, 0xfc, 0x71, 0x40, 0x46, 0xe4,  //
  0x3c, 0xa6, 0x0a, 0xe6, 0xaf, 0xc8, 0xbd, 0x35, 0x1c, 0xf1, 0x63, 0x27, 0xb6, 0x6a, 0x5c, 0x76,  //
};

template<class T, usize N, class U, class Bytes>
void ensure_same(Bytes bytes) {
  using T1 = lps::prelude::vector<T, N>;
  using T2 = lps::generic::vector<T, N>;

  const T1 t1 = std::bit_cast<T1>(bytes);
  const T2 t2 = std::bit_cast<T2>(bytes);

  const auto u1 = t1.template convert<U>().to_array();
  const auto u2 = t2.template convert<U>().to_array();

  REQUIRE(u1 == u2, "{} -> {} (N: {}, signed: {})", sizeof(T), sizeof(U), N, std::is_signed_v<T>);
}

int main() {
  // u8

  ensure_same<u8, 16, i16>(bytes16);
  ensure_same<u8, 16, i32>(bytes16);
  ensure_same<u8, 16, i64>(bytes16);

  ensure_same<u8, 32, i16>(bytes32);
  ensure_same<u8, 32, i32>(bytes32);
  ensure_same<u8, 32, i64>(bytes32);

  ensure_same<u8, 64, i16>(bytes64);
  ensure_same<u8, 64, i32>(bytes64);
  ensure_same<u8, 64, i64>(bytes64);

  ensure_same<u8, 16, u16>(bytes16);
  ensure_same<u8, 16, u32>(bytes16);
  ensure_same<u8, 16, u64>(bytes16);

  ensure_same<u8, 32, u16>(bytes32);
  ensure_same<u8, 32, u32>(bytes32);
  ensure_same<u8, 32, u64>(bytes32);

  ensure_same<u8, 64, u16>(bytes64);
  ensure_same<u8, 64, u32>(bytes64);
  ensure_same<u8, 64, u64>(bytes64);

  // i8

  ensure_same<i8, 16, i16>(bytes16);
  ensure_same<i8, 16, i32>(bytes16);
  ensure_same<i8, 16, i64>(bytes16);

  ensure_same<i8, 32, i16>(bytes32);
  ensure_same<i8, 32, i32>(bytes32);
  ensure_same<i8, 32, i64>(bytes32);

  ensure_same<i8, 64, i16>(bytes64);
  ensure_same<i8, 64, i32>(bytes64);
  ensure_same<i8, 64, i64>(bytes64);

  ensure_same<i8, 16, u16>(bytes16);
  ensure_same<i8, 16, u32>(bytes16);
  ensure_same<i8, 16, u64>(bytes16);

  ensure_same<i8, 32, u16>(bytes32);
  ensure_same<i8, 32, u32>(bytes32);
  ensure_same<i8, 32, u64>(bytes32);

  ensure_same<i8, 64, u16>(bytes64);
  ensure_same<i8, 64, u32>(bytes64);
  ensure_same<i8, 64, u64>(bytes64);

  // u16

  ensure_same<u16, 8, u8>(bytes16);
  ensure_same<u16, 8, u32>(bytes16);
  ensure_same<u16, 8, u64>(bytes16);

  ensure_same<u16, 16, u8>(bytes32);
  ensure_same<u16, 16, u32>(bytes32);
  ensure_same<u16, 16, u64>(bytes32);

  ensure_same<u16, 32, u8>(bytes64);
  ensure_same<u16, 32, u32>(bytes64);
  ensure_same<u16, 32, u64>(bytes64);

  // i16

  ensure_same<i16, 8, i8>(bytes16);
  ensure_same<i16, 8, i32>(bytes16);
  ensure_same<i16, 8, i64>(bytes16);

  ensure_same<i16, 16, i8>(bytes32);
  ensure_same<i16, 16, i32>(bytes32);
  ensure_same<i16, 16, i64>(bytes32);

  ensure_same<i16, 32, i8>(bytes64);
  ensure_same<i16, 32, i32>(bytes64);
  ensure_same<i16, 32, i64>(bytes64);

  // u32

  ensure_same<u32, 4, u8>(bytes16);
  ensure_same<u32, 4, u16>(bytes16);
  ensure_same<u32, 4, u64>(bytes16);

  ensure_same<u32, 8, u8>(bytes32);
  ensure_same<u32, 8, u16>(bytes32);
  ensure_same<u32, 8, u64>(bytes32);

  ensure_same<u32, 16, u8>(bytes64);
  ensure_same<u32, 16, u16>(bytes64);
  ensure_same<u32, 16, u64>(bytes64);

  // i32

  ensure_same<i32, 4, i8>(bytes16);
  ensure_same<i32, 4, i16>(bytes16);
  ensure_same<i32, 4, i64>(bytes16);

  ensure_same<i32, 8, i8>(bytes32);
  ensure_same<i32, 8, i16>(bytes32);
  ensure_same<i32, 8, i64>(bytes32);

  ensure_same<i32, 16, i8>(bytes64);
  ensure_same<i32, 16, i16>(bytes64);
  ensure_same<i32, 16, i64>(bytes64);

  // u64

  ensure_same<u64, 2, u8>(bytes16);
  ensure_same<u64, 2, u16>(bytes16);
  ensure_same<u64, 2, u32>(bytes16);

  ensure_same<u64, 4, u8>(bytes32);
  ensure_same<u64, 4, u16>(bytes32);
  ensure_same<u64, 4, u32>(bytes32);

  ensure_same<u64, 8, u8>(bytes64);
  ensure_same<u64, 8, u16>(bytes64);
  ensure_same<u64, 8, u32>(bytes64);

  // i64

  ensure_same<i64, 2, i8>(bytes16);
  ensure_same<i64, 2, i16>(bytes16);
  ensure_same<i64, 2, i32>(bytes16);

  ensure_same<i64, 4, i8>(bytes32);
  ensure_same<i64, 4, i16>(bytes32);
  ensure_same<i64, 4, i32>(bytes32);

  ensure_same<i64, 8, i8>(bytes64);
  ensure_same<i64, 8, i16>(bytes64);
  ensure_same<i64, 8, i32>(bytes64);
}
