#ifndef IV_PLATFORM_MATH_H_
#define IV_PLATFORM_MATH_H_
#include <cmath>
#include <cassert>
#include <limits>
#include <cfloat>
#include <iv/platform.h>
#include <iv/canonicalized_nan.h>
#include <iv/detail/cmath.h>
namespace iv {
namespace core {
namespace math {

static const double kInfinity = std::numeric_limits<double>::infinity();
static const double kNaN = core::kNaN;

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

inline double Log2(double x) {
  // log2(n) = log(n) / log(2)
  //
  // 0.693147180559945309417232121458176568 is log(2) constant
  // and it isn't provided in default windows math.h
  //
  // http://msdn.microsoft.com/ja-jp/library/4hwaceh6(v=vs.80).aspx
  //
  return std::log(x) / 0.693147180559945309417232121458176568;
}

inline double Log1p(double x) {
  // log1p(n) = log(n + 1)
  if (x == 0) {
    return x;
  }
  return std::log(x + 1);
}

inline double Expm1(double x) {
  // expm1(n) = exp(n) - 1
  //
  // Taylor series
  //   exp(x) = 1 + x + (x^2)/2 + (x^3)*6 + ...
  //
  if (x == 0) {
    return x;
  }
  return std::exp(x) - 1;
}

inline double Acosh(double x) {
  // acosh(x) = log(x + sqrt(x^2 - 1))
  return std::log(x + std::sqrt(x * x - 1));
}

inline double Asinh(double x) {
  // asinh(x) = log(x + sqrt(x^2 + 1))
  if (x == 0 || x == -kInfinity) {
    return x;
  }
  return std::log(x + std::sqrt(x * x + 1));
}

inline double Atanh(double x) {
  // atanh(x) = log((1 + x) / (1 - x)) / 2
  if (x == 0) {
    return x;
  }
  return std::log((1 + x) / (1 - x)) / 2;
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

inline double Log2(double x) {
  return std::log2(x);
}

inline double Log1p(double x) {
  return std::log1p(x);
}

inline double Expm1(double x) {
  return std::expm1(x);
}

inline double Acosh(double x) {
  return std::acosh(x);
}

inline double Asinh(double x) {
  return std::asinh(x);
}

inline double Atanh(double x) {
  return std::atanh(x);
}

#endif

inline double Round(double value) {
  const double res = std::ceil(value);
  if (value > 0) {
    return (res - value > 0.5) ? res - 1 : res;
  } else {
    return (res - value >= 0.5) ? res - 1 : res;
  }
}

// This behavior is based on ECMA262
inline double JSRound(double value) {
  const double res = std::ceil(value);
  return (res - value > 0.5) ? res - 1 : res;
}

inline double Trunc(double value) {
  return (value > 0) ? std::floor(value) : std::ceil(value);
}

inline double Log10(double x) {
  // log10(n) = log(n) / log(10)
  //
  // missing impl
  //   return std::log(x) / std::log(10);
  return std::log10(x);
}

inline double Cosh(double x) {
  // cosh(x) = (exp(x) + exp(-x)) / 2
  //
  // missing impl
  //   return (std::exp(x) + std::exp(-x)) / 2;
  return std::cosh(x);
}

inline double Sinh(double x) {
  // sinh(x) = (exp(x) - exp(-x)) / 2
  //
  // missing impl
  //   if (x == 0 || x == kInfinity || x == -kInfinity) {
  //     // if x is -0, sinh(-0) is -0
  //     return x;
  //   }
  //   return (std::exp(x) - std::exp(-x)) / 2;
  return std::sinh(x);
}

inline double Tanh(double x) {
  // tanh(x) = sinh(x) / cosh(x)
  //
  // missing impl
  //   if (x == kInfinity) {
  //     return 1;
  //   }
  //   if (x == -kInfinity) {
  //     return -1;
  //   }
  //   return Sinh(x) / Cosh(x);
  return std::tanh(x);
}


inline double Hypot(double x, double y) {
  // hypot(x, y)^2 = x^2 + y^2
  // hypot(x, y) = sqrt(x^2 + y^2)
  //
  // but, x*x + y*y is too large. so we transform this expression to
  //
  //  hypot(x, y) = x * sqrt(1.0 + (y/x)^2)
  //
  // http://d.hatena.ne.jp/scior/20101214/1292333501
  // http://d.hatena.ne.jp/rubikitch/20081020/1224507459
  if (Signbit(x)) {
    x = -x;
  }
  if (Signbit(y)) {
    y = -y;
  }

  // NaN check
  if (IsNaN(x)) {
    return IsInf(y) ? kInfinity : kNaN;
  } else if (IsNaN(y)) {
    return IsInf(x) ? kInfinity : kNaN;
  }

  if (y > x) {
    std::swap(x, y);
  }
  assert(x >= y);

  if (x == 0) {
    return 0;
  }

  y /= x;
  return x * std::sqrt(1.0 + y * y);
}

inline double Hypot(double x, double y, double z) {
  // hypot(x, y, z)^2 = x^2 + y^2 + z^2
  // hypot(x, y, z) = sqrt(x^2 + y^2 + z^2)
  //
  // hypot(x, y, z) = x * sqrt(1.0 + (y/x)^2 + (z/x)^2)
  //
  if (Signbit(x)) {
    x = -x;
  }
  if (Signbit(y)) {
    y = -y;
  }
  if (Signbit(z)) {
    z = -z;
  }

  // NaN check
  if (IsNaN(x)) {
    return IsInf(y) || IsInf(z) ? kInfinity : kNaN;
  } else if (IsNaN(y)) {
    return IsInf(x) || IsInf(z) ? kInfinity : kNaN;
  } else if (IsNaN(z)) {
    return IsInf(x) || IsInf(y) ? kInfinity : kNaN;
  }

  if (y > x) {
    if (y > z) {
      std::swap(x, y);
    } else {
      std::swap(x, z);
    }
  } else {
    if (z > x) {
      std::swap(x, z);
    }
  }

  assert(x >= y && x >= z);

  if (x == 0) {
    // all values are 0
    return 0;
  }

  y /= x;
  z /= x;
  return x * std::sqrt(1.0 + y * y + z * z);
}

inline double Hypot2(double x, double y, double z = 0.0) {
  // NaN check
  if (IsNaN(x)) {
    return IsInf(y) || IsInf(z) ? kInfinity : kNaN;
  } else if (IsNaN(y)) {
    return IsInf(x) || IsInf(z) ? kInfinity : kNaN;
  } else if (IsNaN(z)) {
    return IsInf(x) || IsInf(y) ? kInfinity : kNaN;
  }
  return x*x + y*y + z*z;
}

inline double Cbrt(double x) {
  if (fabs(x) < DBL_EPSILON) {
    return 0.0;
  }
  if (x > 0.0) {
    return pow(x, 1.0/3.0);
  }
  return -pow(-x, 1.0/3.0);
}

} } }  // namespace iv::core::math
#endif  // IV_PLATFORM_MATH_H_
