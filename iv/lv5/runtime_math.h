#ifndef IV_LV5_RUNTIME_MATH_H_
#define IV_LV5_RUNTIME_MATH_H_
#include <cmath>
#include <iv/platform_math.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/error.h>

namespace iv {
namespace lv5 {
namespace runtime {
namespace math_detail {

static const double kInfinity = std::numeric_limits<double>::infinity();

}  // namespace math_detail

inline JSVal MathAbs(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.abs", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::abs(x);
  }
  return JSNaN;
}

inline JSVal MathAcos(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.acos", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::acos(x);
  }
  return JSNaN;
}

inline JSVal MathAsin(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.asin", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::asin(x);
  }
  return JSNaN;
}

inline JSVal MathAtan(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atan", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::atan(x);
  }
  return JSNaN;
}

inline JSVal MathAtan2(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atan2", args, e);
  if (args.size() > 1) {
    const double y = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    const double x = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    return std::atan2(y, x);
  }
  return JSNaN;
}

inline JSVal MathCeil(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.ceil", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::ceil(x);
  }
  return JSNaN;
}

inline JSVal MathCos(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.cos", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::cos(x);
  }
  return JSNaN;
}

inline JSVal MathExp(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.exp", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::exp(x);
  }
  return JSNaN;
}

inline JSVal MathFloor(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.floor", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::floor(x);
  }
  return JSNaN;
}

inline JSVal MathLog(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.log", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::log(x);
  }
  return JSNaN;
}

inline JSVal MathMax(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.max", args, e);
  double max = -math_detail::kInfinity;
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

inline JSVal MathMin(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.min", args, e);
  double min = math_detail::kInfinity;
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

inline JSVal MathPow(const Arguments& args, Error* e) {
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

inline JSVal MathRandom(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.random", args, e);
  return args.ctx()->Random();
}

inline JSVal MathRound(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.round", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return core::math::Round(x);
  }
  return JSNaN;
}

inline JSVal MathSin(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sin", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::sin(x);
  }
  return JSNaN;
}

inline JSVal MathSqrt(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sqrt", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::sqrt(x);
  }
  return JSNaN;
}

inline JSVal MathTan(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.tan", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    return std::tan(x);
  }
  return JSNaN;
}

inline JSVal MathTrunc(const Arguments& args, Error* e) {
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

inline JSVal MathSign(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sign", args, e);
  if (!args.empty()) {
    const double x = args.front().ToNumber(args.ctx(), e);
    if (core::math::IsNaN(x) || x == 0) {
      return x;
    }
    return (x < 0) ? -1 : 1;
  }
  return JSNaN;
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MATH_H_
