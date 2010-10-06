#include <limits>
#include "conversions-inl.h"
namespace iv {
namespace core {

const double Conversions::kNaN = std::numeric_limits<double>::quiet_NaN();
const std::string Conversions::kInfinity = "Infinity";

} }  // namespace iv::core
