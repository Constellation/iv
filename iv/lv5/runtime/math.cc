#include <cmath>
#include <iv/platform_math.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/error.h>
#include <iv/lv5/runtime/math.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.8.2.1 abs(x)
JSVal MathAbs(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.abs", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::abs(x);
  }
  return JSNaN;
}

// section 15.8.2.2 acos(x)
JSVal MathAcos(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.acos", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::acos(x);
  }
  return JSNaN;
}

// section 15.8.2.3 asin(x)
JSVal MathAsin(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.asin", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::asin(x);
  }
  return JSNaN;
}

// section 15.8.2.4 atan(x)
JSVal MathAtan(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atan", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::atan(x);
  }
  return JSNaN;
}

// section 15.8.2.5 atan2(y, x)
JSVal MathAtan2(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atan2", args, e);
  if (args.size() > 1) {
    const double y = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    const double x = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    return std::atan2(y, x);
  }
  return JSNaN;
}

// section 15.8.2.6 ceil(x)
JSVal MathCeil(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.ceil", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::ceil(x);
  }
  return JSNaN;
}

// section 15.8.2.7 cos(x)
JSVal MathCos(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.cos", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::cos(x);
  }
  return JSNaN;
}

// section 15.8.2.8 exp(x)
JSVal MathExp(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.exp", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::exp(x);
  }
  return JSNaN;
}

// section 15.8.2.9 floor(x)
JSVal MathFloor(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.floor", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::floor(x);
  }
  return JSNaN;
}

// section 15.8.2.10 log(x)
JSVal MathLog(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.log", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::log(x);
  }
  return JSNaN;
}

// section 15.8.2.11 max([value1[, value2[, ... ]]])
JSVal MathMax(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.max", args, e);
  double max = -core::math::kInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), IV_LV5_ERROR(e));
    if (core::math::IsNaN(x)) {
      return x;
    } else if (x > max || (x == 0.0 && max == 0.0 && !core::math::Signbit(x))) {
      max = x;
    }
  }
  return max;
}

// section 15.8.2.12 min([value1[, value2[, ... ]]])
JSVal MathMin(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.min", args, e);
  double min = core::math::kInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), IV_LV5_ERROR(e));
    if (core::math::IsNaN(x)) {
      return x;
    } else if (x < min || (x == 0.0 && min == 0.0 && core::math::Signbit(x))) {
      min = x;
    }
  }
  return min;
}

// section 15.8.2.13 pow(x, y)
JSVal MathPow(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.pow", args, e);
  if (args.size() > 1) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    const double y = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    if (y == 0) {
      return 1.0;
    } else if (core::math::IsNaN(y) ||
               ((x == 1 || x == -1) && core::math::IsInf(y))) {
      return JSNaN;
    } else {
      return std::pow(x, y);
    }
  }
  return JSNaN;
}

// section 15.8.2.14 random()
JSVal MathRandom(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.random", args, e);
  return args.ctx()->Random();
}

// section 15.8.2.15 round(x)
JSVal MathRound(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.round", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::JSRound(x);
  }
  return JSNaN;
}

// section 15.8.2.16 sin(x)
JSVal MathSin(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sin", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::sin(x);
  }
  return JSNaN;
}

// section 15.8.2.17 sqrt(x)
JSVal MathSqrt(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sqrt", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::sqrt(x);
  }
  return JSNaN;
}

// section 15.8.2.18 tan(x)
JSVal MathTan(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.tan", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::tan(x);
  }
  return JSNaN;
}

// section 15.8.2.19 log10(x)
JSVal MathLog10(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.log10", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Log10(x);
  }
  return JSNaN;
}

// section 15.8.2.20 log2(x)
JSVal MathLog2(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.log2", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Log2(x);
  }
  return JSNaN;
}

// section 15.8.2.21 log1p(x)
JSVal MathLog1p(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.log1p", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Log1p(x);
  }
  return JSNaN;
}

// section 15.8.2.22 expm1(x)
JSVal MathExpm1(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.expm1", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Expm1(x);
  }
  return JSNaN;
}

// section 15.8.2.23 cosh(x)
JSVal MathCosh(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.cosh", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Cosh(x);
  }
  return JSNaN;
}

// section 15.8.2.24 sinh(x)
JSVal MathSinh(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sinh", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Sinh(x);
  }
  return JSNaN;
}

// section 15.8.2.25 tanh(x)
JSVal MathTanh(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.tanh", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Tanh(x);
  }
  return JSNaN;
}

// section 15.8.2.26 acosh(x)
JSVal MathAcosh(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.acosh", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Acosh(x);
  }
  return JSNaN;
}

// section 15.8.2.27 asinh(x)
JSVal MathAsinh(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.asinh", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Asinh(x);
  }
  return JSNaN;
}

// section 15.8.2.28 atanh(x)
JSVal MathAtanh(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atanh", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Atanh(x);
  }
  return JSNaN;
}

// section 15.8.2.29 hypot(value1, value2[, value3])
JSVal MathHypot(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.hypot", args, e);
  if (args.size() > 1) {
    const double value1 = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    const double value2 = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    if (args.size() == 2) {
      return core::math::Hypot(value1, value2);
    } else {
      const double value3 = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      return core::math::Hypot(value1, value2, value3);
    }
  }
  return JSNaN;
}

// section 15.8.2.30 trunc(x)
JSVal MathTrunc(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.trunc", args, e);
  if (!args.empty()) {
    const JSVal target = args.front();
    if (target.IsInt32()) {
      return target;
    }
    const double x = target.ToNumber(args.ctx(), e);
    if (!core::math::IsFinite(x) || x == 0) {
      return x;
    }
    return core::math::Trunc(x);
  }
  return JSNaN;
}

// section 15.8.2.31 sign(x)
JSVal MathSign(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sign", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    if (core::math::IsNaN(x) || x == 0) {
      return x;
    }
    return (core::math::Signbit(x)) ? -1 : 1;
  }
  return JSNaN;
}

// section 15.8.2.32 cbrt(x)
JSVal MathCbrt(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.cbrt", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    if (!core::math::IsFinite(x) || x == 0) {
      return x;
    }
    return core::math::Cbrt(x);
  }
  return JSNaN;
}

JSVal MathImul(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.imul", args, e);
  uint32_t first = 0;
  uint32_t second = 0;
  if (args.IsDefined(0)) {
    first = args[0].ToUInt32(args.ctx(), IV_LV5_ERROR(e));
  }
  if (args.IsDefined(1)) {
    second = args[1].ToUInt32(args.ctx(), IV_LV5_ERROR(e));
  }
  const uint32_t product = first * second;
  const int32_t result =
      product > INT32_MAX ? INT32_MIN + (product - INT32_MAX - 1) : product;
  return JSVal::Int32(result);
}

} } }  // namespace iv::lv5::runtime
