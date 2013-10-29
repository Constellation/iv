#ifndef IV_LV5_RUNTIME_MATH_H_
#define IV_LV5_RUNTIME_MATH_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.8.2.1 abs(x)
JSVal MathAbs(const Arguments& args, Error* e);

// section 15.8.2.2 acos(x)
JSVal MathAcos(const Arguments& args, Error* e);

// section 15.8.2.3 asin(x)
JSVal MathAsin(const Arguments& args, Error* e);

// section 15.8.2.4 atan(x)
JSVal MathAtan(const Arguments& args, Error* e);

// section 15.8.2.5 atan2(y, x)
JSVal MathAtan2(const Arguments& args, Error* e);

// section 15.8.2.6 ceil(x)
JSVal MathCeil(const Arguments& args, Error* e);

// section 15.8.2.7 cos(x)
JSVal MathCos(const Arguments& args, Error* e);

// section 15.8.2.8 exp(x)
JSVal MathExp(const Arguments& args, Error* e);

// section 15.8.2.9 floor(x)
JSVal MathFloor(const Arguments& args, Error* e);

// section 15.8.2.10 log(x)
JSVal MathLog(const Arguments& args, Error* e);

// section 15.8.2.11 max([value1[, value2[, ... ]]])
JSVal MathMax(const Arguments& args, Error* e);

// section 15.8.2.12 min([value1[, value2[, ... ]]])
JSVal MathMin(const Arguments& args, Error* e);

// section 15.8.2.13 pow(x, y)
JSVal MathPow(const Arguments& args, Error* e);

// section 15.8.2.14 random()
JSVal MathRandom(const Arguments& args, Error* e);

// section 15.8.2.15 round(x)
JSVal MathRound(const Arguments& args, Error* e);

// section 15.8.2.16 sin(x)
JSVal MathSin(const Arguments& args, Error* e);

// section 15.8.2.17 sqrt(x)
JSVal MathSqrt(const Arguments& args, Error* e);

// section 15.8.2.18 tan(x)
JSVal MathTan(const Arguments& args, Error* e);

// section 15.8.2.19 log10(x)
JSVal MathLog10(const Arguments& args, Error* e);

// section 15.8.2.20 log2(x)
JSVal MathLog2(const Arguments& args, Error* e);

// section 15.8.2.21 log1p(x)
JSVal MathLog1p(const Arguments& args, Error* e);

// section 15.8.2.22 expm1(x)
JSVal MathExpm1(const Arguments& args, Error* e);

// section 15.8.2.23 cosh(x)
JSVal MathCosh(const Arguments& args, Error* e);

// section 15.8.2.24 sinh(x)
JSVal MathSinh(const Arguments& args, Error* e);

// section 15.8.2.25 tanh(x)
JSVal MathTanh(const Arguments& args, Error* e);

// section 15.8.2.26 acosh(x)
JSVal MathAcosh(const Arguments& args, Error* e);

// section 15.8.2.27 asinh(x)
JSVal MathAsinh(const Arguments& args, Error* e);

// section 15.8.2.28 atanh(x)
JSVal MathAtanh(const Arguments& args, Error* e);

// section 15.8.2.29 hypot(value1, value2[, value3])
JSVal MathHypot(const Arguments& args, Error* e);

// section 15.8.2.30 trunc(x)
JSVal MathTrunc(const Arguments& args, Error* e);

// section 15.8.2.31 sign(x)
JSVal MathSign(const Arguments& args, Error* e);

// section 15.8.2.32 cbrt(x)
JSVal MathCbrt(const Arguments& args, Error* e);

JSVal MathImul(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MATH_H_
