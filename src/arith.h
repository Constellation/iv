// arithmetic functions
#ifndef IV_ARITH_H_
#define IV_ARITH_H_
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

} }  // namespace iv::core
#endif  // IV_ARITH_H_
