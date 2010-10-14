#include "utils.h"
#include <cmath>

namespace iv {
namespace core {
namespace {
  static double DoubleToInt32_Two32 = 4294967296.0;
  static double DoubleToInt32_Two31 = 2147483648.0;
}

int32_t Conv::DoubleToInt32(double d) {
  int32_t i = static_cast<int32_t>(d);
  if (static_cast<double>(i) == d) {
    return i;
  }
  if (!std::isfinite(d) || d == 0) {
    return 0;
  }
  if (d < 0 || d >= DoubleToInt32_Two32) {
    d = std::fmod(d, DoubleToInt32_Two32);
  }
  d = (d >= 0) ? std::floor(d) : std::ceil(d) + DoubleToInt32_Two32;
  return static_cast<int32_t>(d >= DoubleToInt32_Two31 ? d - DoubleToInt32_Two32 : d);
}

} }  // namespace iv::core
