#ifndef _IV_LV5_RUNTIME_NUMBER_H_
#define _IV_LV5_RUNTIME_NUMBER_H_
#include <cstdlib>
#include <tr1/array>
#include <tr1/cmath>
#include <tr1/cstdio>
#include <tr1/cinttypes>
#include "dtoa.h"
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/math.h"
#include "lv5/jsobject.h"
#include "lv5/jsstring.h"
#include "lv5/jsdtoa.h"
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.7.1.1 Number([value])
// section 15.7.2.1 new Number([value])
inline JSVal NumberConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    double res = 0.0;
    if (args.size() > 0) {
      res = args[0].ToNumber(args.ctx(), ERROR(error));
    }
    return JSNumberObject::New(args.ctx(), res);
  } else {
    if (args.size() > 0) {
      return args[0].ToNumber(args.ctx(), error);
    } else {
      return 0.0;
    }
  }
}

// section 15.7.4.2 Number.prototype.toString([radix])
inline JSVal NumberToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("Number").name == obj.object()->class_name()) {
      num = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toString is not generic function");
      return JSUndefined;
    }
  } else {
    num = obj.number();
  }
  if (args.size() > 0) {
    const JSVal& first = args[0];
    double radix;
    if (first.IsUndefined()) {
      radix = 10;
    } else {
      radix = first.ToNumber(args.ctx(), ERROR(error));
      radix = core::DoubleToInteger(radix);
    }
    if (2 <= radix && radix <= 36) {
      // if radix == 10, through to radix 10 or no radix
      if (radix != 10) {
        if (std::isnan(num)) {
          return JSString::NewAsciiString(args.ctx(), "NaN");
        } else if (!std::isfinite(num)) {
          if (num > 0) {
            return JSString::NewAsciiString(args.ctx(), "Infinity");
          } else {
            return JSString::NewAsciiString(args.ctx(), "-Infinity");
          }
        }
        StringBuilder builder;
        core::DoubleToStringWithRadix(
            num,
            static_cast<int>(radix),
            &builder);
        return builder.Build(args.ctx());
      }
    } else {
      // TODO(Constellation) more details
      error->Report(Error::Range,
                    "illegal radix");
      return JSUndefined;
    }
  }
  // radix 10 or no radix
  std::tr1::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str);
}

// section 15.7.4.2 Number.prototype.toLocaleString()
inline JSVal NumberToLocaleString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.toLocaleString", args, error);
  const JSVal& obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("Number").name == obj.object()->class_name()) {
      num = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toLocaleString is not generic function");
      return JSUndefined;
    }
  } else {
    num = obj.number();
  }
  std::tr1::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str);
}

// section 15.7.4.4 Number.prototype.valueOf()
inline JSVal NumberValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.valueOf", args, error);
  const JSVal& obj = args.this_binding();
  if (!obj.IsNumber()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("Number").name == obj.object()->class_name()) {
      return static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.valueOf is not generic function");
      return JSUndefined;
    }
  } else {
    return obj.number();
  }
}

// section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
inline JSVal NumberToFixed(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.toFixed", args, error);
  const std::size_t arg_count = args.size();
  Context* const ctx = args.ctx();
  double fd;
  if (arg_count == 0) {
    fd = 0.0;
  } else {
    const JSVal& first = args[0];
    if (first.IsUndefined()) {
      fd = 0.0;
    } else {
      fd = first.ToNumber(ctx, ERROR(error));
      fd = core::DoubleToInteger(fd);
    }
  }
  if (fd < 0 || fd > 20) {
    error->Report(Error::Range,
                  "fractionDigits is in range between 0 to 20");
    return JSUndefined;
  }
  const int f = core::DoubleToInt32(fd);
  const JSVal& obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("Number").name == obj.object()->class_name()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toFixed is not generic function");
      return JSUndefined;
    }
  } else {
    x = obj.number();
  }
  if (!(std::fabs(x) < 1e+21)) {
    // included NaN and Infinity
    std::tr1::array<char, 80> buffer;
    const char* const str = core::DoubleToCString(x,
                                                  buffer.data(),
                                                  buffer.size());
    return JSString::NewAsciiString(args.ctx(), str);
  } else {
    // TODO(Constellation) (0.5).toFixed(0) === 1
    return DoubleToJSString<DTOA_FIXED>(ctx, x, f, 0);
  }
}

// section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
inline JSVal NumberToExponential(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.toExponential", args, error);
  const std::size_t arg_count = args.size();
  Context* const ctx = args.ctx();

  const JSVal& obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("Number").name == obj.object()->class_name()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toExponential is not generic function");
      return JSUndefined;
    }
  } else {
    x = obj.number();
  }

  if (!std::isfinite(x)) {
    if (std::isnan(x)) {
      return JSString::NewAsciiString(ctx, "NaN");
    } else {
      if (x < 0) {
        return JSString::NewAsciiString(ctx, "-Infinity");
      } else {
        return JSString::NewAsciiString(ctx, "Infinity");
      }
    }
  }

  JSVal fractionDigits;
  double fd;
  if (arg_count == 0) {
    fd = 0.0;
  } else {
    fractionDigits = args[0];
    if (fractionDigits.IsUndefined()) {
      fd = 0.0;
    } else {
      fd = fractionDigits.ToNumber(ctx, ERROR(error));
      fd = core::DoubleToInteger(fd);
    }
  }
  if (!fractionDigits.IsUndefined() &&
      (fd < 0 || fd > 20)) {
    error->Report(Error::Range,
                  "fractionDigits is in range between 0 to 20");
    return JSUndefined;
  }
  const int f = core::DoubleToInt32(fd);

  if (fractionDigits.IsUndefined()) {
    return DoubleToJSString<DTOA_STD_EXPONENTIAL>(ctx, x, f, 1);
  } else {
    return DoubleToJSString<DTOA_EXPONENTIAL>(ctx, x, f, 1);
  }
}

// section 15.7.4.7 Number.prototype.toPrecision(precision)
inline JSVal NumberToPrecision(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.toPrecision", args, error);
  const std::size_t arg_count = args.size();
  Context* const ctx = args.ctx();

  const JSVal& obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("Number").name == obj.object()->class_name()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toPrecision is not generic function");
      return JSUndefined;
    }
  } else {
    x = obj.number();
  }

  double p;
  if (arg_count == 0) {
    return obj.ToString(ctx, error);
  } else {
    const JSVal& precision = args[0];
    if (precision.IsUndefined()) {
      return obj.ToString(ctx, error);
    } else {
      p = precision.ToNumber(ctx, ERROR(error));
      p = core::DoubleToInteger(p);
    }
  }

  if (!std::isfinite(x)) {
    if (std::isnan(x)) {
      return JSString::NewAsciiString(ctx, "NaN");
    } else {
      if (x < 0) {
        return JSString::NewAsciiString(ctx, "-Infinity");
      } else {
        return JSString::NewAsciiString(ctx, "Infinity");
      }
    }
  }

  if (p < 1 || p > 21) {
    error->Report(Error::Range,
                  "precision is in range between 1 to 21");
    return JSUndefined;
  }

  return DoubleToJSString<DTOA_PRECISION>(ctx, x, core::DoubleToInt32(p), 0);
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_NUMBER_H_
