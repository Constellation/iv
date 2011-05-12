#ifndef _IV_PLATFORM_MATH_H_
#define _IV_PLATFORM_MATH_H_
#include "platform.h"
#if defined(COMPILER_MSVC)
namespace iv {
namespace core {

// http://msdn.microsoft.com/library/tzthab44.aspx
inline int IsNaN(const double& val) {
  return _isnan(val);
}

// http://msdn.microsoft.com/library/sb8es7a8.aspx
inline int IsFinite(const double& val) {
  return _finite(val);
}

} }  // namespace iv::core
#else
#include <cmath>
namespace iv {
namespace core {

inline int IsNaN(const double& val) {
  return std::isnan(val);
}

inline int IsFinite(const double& val) {
  return std::isfinite(val);
}

} }  // namespace iv::core
#endif
#endif  // _IV_PLATFORM_MATH_H_
