// arithmetic functions
#ifndef IV_ARITH_H_
#define IV_ARITH_H_
#include <iv/detail/array.h>
#include <iv/detail/cinttypes.h>
#include <iv/platform.h>
#ifdef IV_OS_WIN
  #include <intrin.h>
#endif
namespace iv {
namespace core {
namespace math {
namespace detail {

template<uint32_t X, uint32_t N>
struct CLP2ST {
  static const uint32_t value = X | (X >> N);
};

template<uint32_t X>
struct CLP2 {
  static const uint32_t value =
      CLP2ST<
        CLP2ST<
          CLP2ST<
            CLP2ST<
              CLP2ST<X - 1, 1>::value,
              2>::value,
            4>::value,
          8>::value,
      16>::value + 1;
};

template<uint32_t X>
struct FLP2 {
  static const uint32_t x =
      CLP2ST<
        CLP2ST<
          CLP2ST<
            CLP2ST<
              CLP2ST<X, 1>::value,
              2>::value,
            4>::value,
          8>::value,
      16>::value;
  static const uint32_t value = x - (x >> 1);
};

template<uint32_t N>
struct NTZTable;

#define V(I, N)\
  template<>struct NTZTable<I>{\
    static const int value = N;\
  };
#define NTZ_TABLE(V)\
V( 0,  0) V( 1,  1) V( 2, 59) V( 3,  2) V( 4, 60) V( 5, 40) V( 6, 54) V( 7,  3)\
V( 8, 61) V( 9, 32) V(10, 49) V(11, 41) V(12, 55) V(13, 19) V(14, 35) V(15,  4)\
V(16, 62) V(17, 52) V(18, 30) V(19, 33) V(20, 50) V(21, 12) V(22, 14) V(23, 42)\
V(24, 56) V(25, 16) V(26, 27) V(27, 20) V(28, 36) V(29, 23) V(30, 44) V(31,  5)\
V(32, 63) V(33, 58) V(34, 39) V(35, 53) V(36, 31) V(37, 48) V(38, 18) V(39, 34)\
V(40, 51) V(41, 29) V(42, 11) V(43, 13) V(44, 15) V(45, 26) V(46, 22) V(47, 43)\
V(48, 57) V(49, 38) V(50, 47) V(51, 17) V(52, 28) V(53, 10) V(54, 25) V(55, 21)\
V(56, 37) V(57, 46) V(58,  9) V(59, 24) V(60, 45) V(61,  8) V(62,  7) V(63,  6)
NTZ_TABLE(V)
#undef V
#undef NTZ_TABLE

template<int64_t X>
struct NTZ {
  static const int value = (!X) ? 64 : NTZTable<(static_cast<uint64_t>(X & -X) * UINT64_C(0x03F566ED27179461) >> 58)>::value;
};

}  // namespace detail

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

// PopCount: population count
// from "Hacker's Delight" section 5-1
inline uint32_t PopCount(uint32_t x) {
#if defined(IV_COMPILER_CLANG) || defined(IV_COMPILER_GCC)
  return __builtin_popcount(x);
#else
  x = (x & 0x55555555) + (x >> 1 & 0x55555555);
  x = (x & 0x33333333) + (x >> 2 & 0x33333333);
  x = (x & 0x0F0F0F0F) + (x >> 4 & 0x0F0F0F0F);
  x = (x & 0x00FF00FF) + (x >> 8 & 0x00FF00FF);
  return (x & 0x0000FFFF) + (x >>16 & 0x0000FFFF);
#endif
}

// from "Hacker's Delight" section 3-2
// flooring to 2^n
inline uint32_t FLP2(uint32_t x) {
#if defined(IV_OS_WIN)
  unsigned long ret;
  if (_BitScanReverse(&ret, x)) {
    return 1U << ret;
  } else {
    return 0;
  }
#elif defined(IV_COMPILER_CLANG) || defined(IV_COMPILER_GCC)
  if (x == 0) return 0;
  return 1U << (__builtin_clz(x) ^ 0x1f);
#else
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x - (x >> 1);
#endif
}

// ceiling to 2^n
inline uint32_t CLP2(uint32_t x) {
#if 1
  if (x == 0 || x > 0x80000000) return 0;
  return FLP2(2 * x - 1);
#else
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
#endif
}

// NLZ: number of leading zeros
// a.k.a. CLZ, count leading zeros
// from "Hacker's Delight" section 5-3
inline uint32_t NLZ(uint32_t x) {
#if defined(IV_COMPILER_CLANG) || defined(IV_COMPILER_GCC)
  return __builtin_clz(x);
#else
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return PopCount(~x);
#endif
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

// too big m is not accepted
inline std::size_t Ceil(std::size_t n, std::size_t m) {
  return (n + m - 1) / m * m;
}

} } }  // namespace iv::core::math
#endif  // IV_ARITH_H_
