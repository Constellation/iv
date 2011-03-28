#ifndef _IV_LV5_MATH_H_
#define _IV_LV5_MATH_H_
#include <cmath>
namespace iv {
namespace lv5 {

inline double Round(const double& value) {
  const double res = std::ceil(value);
  if (res - value > 0.5) {
    return res - 1;
  } else {
    return res;
  }
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_MATH_H_
