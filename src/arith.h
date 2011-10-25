// arithmetic functions
#ifndef IV_ARITH_H_
#define IV_ARITH_H_
#include "detail/cinttypes.h"
namespace iv {
namespace core {

// from "Hacker's Delight" section 2-12
inline bool IsAdditionOverflow(int32_t lhs, int32_t rhs) {
  const int32_t sum = lhs + rhs;
  return ((sum ^ lhs) & (sum ^ rhs)) & 0x80000000;
}

inline bool IsAdditionOverflow(int32_t lhs, int32_t rhs, int32_t* sum) {
  *sum = lhs + rhs;
  return (((*sum) ^ lhs) & ((*sum) ^ rhs)) & 0x80000000;
}

inline bool IsSubtractOverflow(int32_t lhs, int32_t rhs) {
  const int32_t dif = lhs - rhs;
  return ((dif ^ lhs) & (dif ^ rhs)) & 0x80000000;
}

inline bool IsSubtractOverflow(int32_t lhs, int32_t rhs, int32_t* dif) {
  *dif = lhs - rhs;
  return (((*dif) ^ lhs) & ((*dif) ^ rhs)) & 0x80000000;
}

// from "Hacker's Delight" section 3-2
// flooring to 2^n
inline uint32_t FLP2(uint32_t x) {
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x - (x >> 1);
}

// ceiling to 2^n
inline uint32_t CLP2(uint32_t x) {
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

// NTZ: number of tailing zeros
// faster way than "Hacker's Delight" section 5-4
// http://www-graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
// http://slashdot.jp/~Tellur52/journal/448479
// http://d.hatena.ne.jp/siokoshou/20090704#p1
//
// using perfect hashing
//
// table creation code is following
//   #include <cstdint>
//   #include <iostream>
//   int main(int argc, char** argv) {
//     static int* table = new int[ 64 ];
//     uint64_t hash = 0x03F566ED27179461UL;
//     for (int i = 0; i < 64; ++i) {
//       table[hash >> 58] = i;
//       hash <<= 1;
//     }
//     for (int i = 0; i < 64; i++) {
//       std::cout << table[i];
//       if (i % 16 != 15) {
//         std::cout << ", ";
//       } else if (i == 63) {
//         std::cout << std::endl;
//       } else {
//         std::cout << "," << std::endl;
//       }
//     }
//     std::cout << std::endl;
//     return 0;
//   }
static const std::array<uint8_t, 64> kNTZ64 = { {
  0,   1, 59,  2, 60, 40, 54,  3, 61, 32, 49, 41, 55, 19, 35,  4,
  62, 52, 30, 33, 50, 12, 14, 42, 56, 16, 27, 20, 36, 23, 44,  5,
  63, 58, 39, 53, 31, 48, 18, 34, 51, 29, 11, 13, 15, 26, 22, 43,
  57, 38, 47, 17, 28, 10, 25, 21, 37, 46,  9, 24, 45,  8,  7,  6
} };

inline uint64_t NTZ64(int64_t x) {
  if (!x) {
    return 64;
  }
  return kNTZ64[
      static_cast<uint64_t>(x & -x) * UINT64_C(0x03F566ED27179461) >> 58];
}


} }  // namespace iv::core
#endif  // IV_ARITH_H_
