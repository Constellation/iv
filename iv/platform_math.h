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

inline double Log10(double x) {
  // log10(n) = log(n) / log(10)
  return std::log(x) / std::log(10);
}

inline double Log2(double x) {
  // log2(n) = log(n) / log(2)
  return std::log(x) / std::log(2);
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
  if (x == 0) {
    return x;
  }
  return std::exp(x) - 1;
}

inline double Cosh(double x) {
  // cosh(x) = (exp(x) + exp(-x)) / 2
  return (std::exp(x) + std::exp(-x)) / 2;
}

inline double Sinh(double x) {
  // sinh(x) = (exp(x) - exp(-x)) / 2
  if (x == 0 || x == kInfinity || x == -kInfinity) {
    // if x is -0, sinh(-0) is -0
    return x;
  }
  return (std::exp(x) - std::exp(-x)) / 2;
}

inline double Tanh(double x) {
  // tanh(x) = sinh(x) / cosh(x)
  if (x == kInfinity) {
    return 1;
  }
  if (x == -kInfinity) {
    return -1;
  }
  return Sinh(x) / Cosh(x);
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
  if (x < 0) {
    x = -x;
  }
  if (y < 0) {
    y = -y;
  }
  if (y > x) {
    std::swap(y, x);
  }
  if (y == 0) {
    if (x == 0) {
      // if x and y are -0, hypot(-0, -0) returns +0
      return 0;
    }
    return x;
  }
  y /= x;
  return x * std::sqrt(1.0 + y * y);
}

} } }  // namespace iv::core::math
#endif  // IV_PLATFORM_MATH_H_
