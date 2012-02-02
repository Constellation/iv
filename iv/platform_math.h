#ifndef IV_PLATFORM_MATH_H_
#define IV_PLATFORM_MATH_H_
#include <cmath>
#include <limits>
#include <iv/platform.h>
namespace iv {
namespace core {
namespace math {

static const double kInfinity = std::numeric_limits<double>::infinity();

#if defined(IV_COMPILER_MSVC)

// http://msdn.microsoft.com/library/tzthab44.aspx
inline int IsNaN(double val) {
  return _isnan(val);
}

// http://msdn.microsoft.com/library/sb8es7a8.aspx
inline int IsFinite(double val) {
  return _finite(val);
}

inline bool Signbit(double x) {
  return (x == 0) ? (_fpclass(x) & _FPCLASS_NZ) : (x < 0);
}

inline int IsInf(double x) {
  return (_fpclass(x) & (_FPCLASS_PINF | _FPCLASS_NINF)) != 0;
}

inline double Modulo(double x, double y) {
  // std::fmod in Windows is not compatible with ECMA262 modulo
  // in ECMA262
  //
  //   x[finite] / y[infinity] => x
  //   x[+-0] / y[finite, but not 0] => x
  //
  // sputniktests failed example with using std::fmod in Windows:
  //   1 % -Infinity should be 1 (but NaN)
  //   -0.0 % 1 should be -0.0 (but +0.0)
  //
  if (!(IsFinite(x) && IsInf(y)) && !(x == 0 && (y != 0 && IsFinite(y)))) {
    return std::fmod(x, y);
  }
  return x;
}

#else

inline int IsNaN(double val) {
  return std::isnan(val);
}

inline int IsFinite(double val) {
  return std::isfinite(val);
}

inline bool Signbit(double x) {
  return std::signbit(x);
}

inline int IsInf(double x) {
  return std::isinf(x);
}

inline double Modulo(double x, double y) {
  return std::fmod(x, y);
}

#endif

inline double Round(double value) {
  const double res = std::ceil(value);
  if (res - value > 0.5) {
    return res - 1;
  } else {
    return res;
  }
}

inline double Trunc(double value) {
  if (value > 0) {
    return std::floor(value);
  } else {
    return std::ceil(value);
  }
}

} } }  // namespace iv::core::math
#endif  // IV_PLATFORM_MATH_H_
