#ifndef _IV_LV5_RUNTIME_NUMBER_H_
#define _IV_LV5_RUNTIME_NUMBER_H_
#include <tr1/array>
#include <tr1/cmath>
#include <tr1/cstdio>
#include <tr1/cinttypes>
#include "conversions.h"
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsobject.h"
#include "jsstring.h"
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
        JSStringBuilder builder(args.ctx());
        core::DoubleToStringWithRadix(
            num,
            static_cast<int>(radix),
            &builder);
        return builder.Build();
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
  uint32_t f = core::DoubleToUInt32(fd);
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
    // TODO(Constellation) more fast way
    JSStringBuilder builder(ctx);
    if (x < 0) {
      builder.Append('-');
      x = -x;
    }

    std::tr1::array<char, 80> buf;
    const double integer_temp = std::floor(x);
    double decimal;
    uint32_t num;
    if (integer_temp > ((1ULL << 63) - 1)) {
      double integer = integer_temp;
      const double power = std::pow(10, f);
      const double decimal_with_power = (x - integer_temp) * power;
      const double rounded_decimal = std::tr1::round(decimal_with_power);
      if (rounded_decimal == power) {
        integer += 1;
        decimal = 0.0;
      } else {
        decimal = rounded_decimal;
      }
      const char* const str = core::DoubleToCString(integer,
                                                    buf.data(),
                                                    buf.size());
      num = std::strlen(str);
    } else {
      // more precise in uint64_t
      uint64_t integer = core::DoubleToUInt64(integer_temp);
      const double power = std::pow(10, f);
      const double decimal_with_power = (x - integer_temp) * power;
      const double rounded_decimal = std::tr1::round(decimal_with_power);
      if (rounded_decimal == power) {
        integer += 1;
        decimal = 0.0;
      } else {
        decimal = rounded_decimal;
      }
      num = std::tr1::snprintf(buf.data(), buf.size(),
                               "%"PRIu64, integer);
    }
    builder.Append(core::StringPiece(buf.data(), num));
    if (f != 0) {
      builder.Append('.');
      const char* const str = core::DoubleToCString(decimal,
                                                    buf.data(),
                                                    buf.size());
      num = std::strlen(str);
      if (num <= f) {
        using std::fill_n;
        fill_n(buf.data() + num, f + 1 - num, '0');
      }
      builder.Append(core::StringPiece(buf.data(), f));
    }
    return builder.Build();
  }
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_NUMBER_H_
