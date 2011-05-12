#ifndef _IV_PLATFORM_MATH_H_
#define _IV_PLATFORM_MATH_H_
#include "platform.h"
#if defined(COMPILER_MSVC)
namespace iv {
namespace core {

// http://msdn.microsoft.com/library/tzthab44.aspx
inline int IsNaN(double val) {
  return _isnan(val);
}

// http://msdn.microsoft.com/library/sb8es7a8.aspx
inline int IsFinite(double val) {
  return _finite(val);
}

inline int Signbit(double x) {
  return (x == 0) ? (_fpclass(x) & _FPCLASS_NZ) : (x < 0);
}

inline int IsInf(double x) {
  return (_fpclass(x) & (_FPCLASS_PINF | _FPCLASS_NINF)) != 0;
}

} }  // namespace iv::core
#else
#include <cmath>
namespace iv {
namespace core {

inline int IsNaN(double val) {
  return std::isnan(val);
}

inline int IsFinite(double val) {
  return std::isfinite(val);
}

inline int Signbit(double x) {
  return std::signbit(x);
}

inline int IsInf(double x) {
  return std::isinf(x);
}

} }  // namespace iv::core
#endif
#endif  // _IV_PLATFORM_MATH_H_
