// arithmetic functions
#ifndef _IV_ARITH_H_
#define _IV_ARITH_H_
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

inline bool IsMultiplyOverflow(int32_t lhs, int32_t rhs) {
  return true;
}

inline bool IsMultiplyOverflow(int32_t lhs, int32_t rhs, int32_t* sum) {
  return true;
}

} }  // namespace iv::core
#endif  // _IV_ARITH_H_
