#ifndef _IV_LV5_MATH_H_
#define _IV_LV5_MATH_H_
#include <cmath>
namespace iv {
namespace lv5 {

static inline bool IsInfinity(double val) {
  return !std::isfinite(val) && !std::isnan(val);
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_MATH_H_
