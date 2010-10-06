#ifndef IV_CONVERSIONS_H_
#define IV_CONVERSIONS_H_
#include <string>
#include "ustringpiece.h"
namespace iv {
namespace core {

class Conversions {
 public:
  static const double kNaN;
  static const int kMaxSignificantDigits = 772;
  static const std::string kInfinity;
};

} }  // namespace iv::core
#endif  // IV_CONVERSIONS_H_
