#include <limits>
#include "conversions-inl.h"
namespace iv {
namespace core {

const double Conversions::kNaN = std::numeric_limits<double>::quiet_NaN();
const std::string Conversions::kInfinity = "Infinity";
const double Conversions::DoubleToInt32_Two32 = 4294967296.0;
const double Conversions::DoubleToInt32_Two31 = 2147483648.0;

} }  // namespace iv::core
