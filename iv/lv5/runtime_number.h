#ifndef IV_LV5_RUNTIME_NUMBER_H_
#define IV_LV5_RUNTIME_NUMBER_H_
#include <cstdlib>
#include <iv/detail/array.h>
#include <iv/detail/cstdint.h>
#include <iv/conversions.h>
#include <iv/platform_math.h>
#include <iv/arith.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsnumberobject.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsdtoa.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.7.1.1 Number([value])
// section 15.7.2.1 new Number([value])
inline JSVal NumberConstructor(const Arguments& args, Error* e) {
  if (args.IsConstructorCalled()) {
    double res = 0.0;
    if (!args.empty()) {
      res = args.front().ToNumber(args.ctx(), IV_LV5_ERROR(e));
    }
    return JSNumberObject::New(args.ctx(), res);
  } else {
    if (args.empty()) {
      return JSVal::Int32(0);
    } else {
      return args.front().ToNumber(args.ctx(), e);
    }
  }
}

// section 15.7.3.7 Number.isNaN(number)
inline JSVal NumberIsNaN(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.isNaN", args, e);
  const JSVal num = args.At(0);
  if (!num.IsNumber()) {
    return JSFalse;
  }
  if (num.IsInt32()) {
    return JSFalse;
  }
  return JSVal::Bool(core::math::IsNaN(num.number()));
}

// section 15.7.3.8 Number.isFinite(number)
inline JSVal NumberIsFinite(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.isFinite", args, e);
  const JSVal num = args.At(0);
  if (!num.IsNumber()) {
    return JSFalse;
  }
  if (num.IsInt32()) {
    return JSTrue;
  }
  return JSVal::Bool(core::math::IsFinite(num.number()));
}

// section 15.7.3.9 Number.isInteger(number)
inline JSVal NumberIsInteger(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.isInteger", args, e);
  const JSVal num = args.At(0);
  if (!num.IsNumber()) {
    return JSFalse;
  }
  if (num.IsInt32()) {
    return JSTrue;
  }
  const double n = num.number();
  return JSVal::Bool(core::DoubleToInteger(n) == n);
}

// section 15.7.3.10 Number.toInteger(number)
inline JSVal NumberToInt(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.toInteger", args, e);
  const JSVal num = args.At(0);
  if (num.IsInt32()) {
    return num;
  }
  return num.ToInteger(args.ctx(), e);
}

// section 15.7.4.2 Number.prototype.toString([radix])
inline JSVal NumberToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toString", args, e);
  const JSVal obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      num = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toString is not generic function");
      return JSEmpty;
    }
  } else {
    num = obj.number();
  }
  if (!args.empty()) {
    const JSVal first = args[0];
    double radix;
    if (first.IsUndefined()) {
      radix = 10;
    } else {
      radix = first.ToInteger(args.ctx(), IV_LV5_ERROR(e));
    }
    if (2 <= radix && radix <= 36) {
      // if radix == 10, through to radix 10 or no radix
      if (radix != 10) {
        if (core::math::IsNaN(num)) {
          return JSString::NewAsciiString(args.ctx(), "NaN", e);
        } else if (core::math::IsInf(num)) {
          if (num > 0) {
            return JSString::NewAsciiString(args.ctx(), "Infinity", e);
          } else {
            return JSString::NewAsciiString(args.ctx(), "-Infinity", e);
          }
        }
        JSStringBuilder builder;
        core::DoubleToStringWithRadix(
            num,
            static_cast<int>(radix),
            std::back_inserter(builder));
        return builder.Build(args.ctx(), true, e);
      }
    } else {
      e->Report(Error::Range, "illegal radix");
      return JSEmpty;
    }
  }
  // radix 10 or no radix
  std::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str, e);
}

// section 15.7.4.2 Number.prototype.toLocaleString()
// i18n patched version
// section 13.2.1 Number.prototype.toLocaleString([locales [, options]])
inline JSVal NumberToLocaleString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toLocaleString", args, e);
  const JSVal obj = args.this_binding();
  Context* const ctx = args.ctx();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      num = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toLocaleString is not generic function");
      return JSEmpty;
    }
  } else {
    num = obj.number();
  }
  const JSVal locales = args.At(0);
  const JSVal options = args.At(1);
  JSObject* format = JSObject::New(ctx, ctx->global_data()->number_format_map());
  i18n::InitializeNumberFormat(ctx, format, locales, options, IV_LV5_ERROR(e));
  const PropertyDescriptor desc =
      format->GetOwnProperty(ctx, ctx->i18n()->symbols().initializedNumberFormat());
  i18n::JSNumberFormatHolder* f =
      static_cast<i18n::JSNumberFormatHolder*>(desc.AsDataDescriptor()->value().object());
  return f->Format(ctx, num, e);
}

// section 15.7.4.4 Number.prototype.valueOf()
inline JSVal NumberValueOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.valueOf", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      return static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.valueOf is not generic function");
      return JSEmpty;
    }
  } else {
    return obj.number();
  }
}

// section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
inline JSVal NumberToFixed(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toFixed", args, e);
  Context* const ctx = args.ctx();
  double fd;
  if (args.empty()) {
    fd = 0.0;
  } else {
    const JSVal first = args[0];
    if (first.IsUndefined()) {
      fd = 0.0;
    } else {
      fd = first.ToInteger(ctx, IV_LV5_ERROR(e));
    }
  }
  if (fd < 0 || fd > 20) {
    e->Report(Error::Range,
              "fractionDigits is in range between 0 to 20");
    return JSEmpty;
  }
  const JSVal obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toFixed is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.number();
  }
  if (!(std::fabs(x) < 1e+21)) {
    // included NaN and Infinity
    std::array<char, 80> buffer;
    const char* const str = core::DoubleToCString(x,
                                                  buffer.data(),
                                                  buffer.size());
    return JSString::NewAsciiString(args.ctx(), str, e);
  } else {
    JSStringDToA builder(ctx);
    return builder.BuildFixed(x, core::DoubleToInt32(fd), 0);
  }
}

// section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
inline JSVal NumberToExponential(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toExponential", args, e);
  Context* const ctx = args.ctx();

  const JSVal obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toExponential is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.number();
  }

  if (core::math::IsNaN(x)) {
    return JSString::NewAsciiString(ctx, "NaN", e);
  } else if (core::math::IsInf(x)) {
    if (x < 0) {
      return JSString::NewAsciiString(ctx, "-Infinity", e);
    } else {
      return JSString::NewAsciiString(ctx, "Infinity", e);
    }
  }

  JSVal fractionDigits;
  double fd;
  if (args.empty()) {
    fd = 0.0;
  } else {
    fractionDigits = args[0];
    if (fractionDigits.IsUndefined()) {
      fd = 0.0;
    } else {
      fd = fractionDigits.ToInteger(ctx, IV_LV5_ERROR(e));
    }
  }
  if (!fractionDigits.IsUndefined() && (fd < 0 || fd > 20)) {
    e->Report(Error::Range,
              "fractionDigits is in range between 0 to 20");
    return JSEmpty;
  }

  int f = core::DoubleToInt32(fd);
  JSStringDToA builder(ctx);
  if (fractionDigits.IsUndefined()) {
    return builder.BuildStandardExponential(x);
  } else {
    // In ECMA262 5.1th on 2012/03/10, below step is defined,
    // but probably, this is bug, so we remove this specialization.
    // see https://mail.mozilla.org/pipermail/es-discuss/2012-March/021085.html
    //
    // if (x == 0) {
    //   f = 0;
    // }
    return builder.BuildExponential(x, f, 1);
  }
}

// section 15.7.4.7 Number.prototype.toPrecision(precision)
inline JSVal NumberToPrecision(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.toPrecision", args, e);
  Context* const ctx = args.ctx();

  const JSVal obj = args.this_binding();
  double x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = static_cast<JSNumberObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Number.prototype.toPrecision is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.number();
  }

  double p;
  if (args.empty()) {
    return obj.ToString(ctx, e);
  } else {
    const JSVal precision = args[0];
    if (precision.IsUndefined()) {
      return obj.ToString(ctx, e);
    } else {
      p = precision.ToInteger(ctx, IV_LV5_ERROR(e));
    }
  }

  if (core::math::IsNaN(x)) {
    return JSString::NewAsciiString(ctx, "NaN", e);
  } else if (core::math::IsInf(x)) {
    if (x < 0) {
      return JSString::NewAsciiString(ctx, "-Infinity", e);
    } else {
      return JSString::NewAsciiString(ctx, "Infinity", e);
    }
  }

  if (p < 1 || p > 21) {
    e->Report(Error::Range,
              "precision is in range between 1 to 21");
    return JSEmpty;
  }

  JSStringDToA builder(ctx);
  return builder.BuildPrecision(x, core::DoubleToInt32(p), 0);
}

// section 15.7.4.8 Number.prototype.clz()
inline JSVal NumberCLZ(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Number.prototype.clz", args, e);

  const JSVal obj = args.this_binding();
  uint32_t x;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Number>()) {
      x = core::DoubleToUInt32(static_cast<JSNumberObject*>(obj.object())->value());
    } else {
      e->Report(Error::Type, "Number.prototype.clz is not generic function");
      return JSEmpty;
    }
  } else {
    x = obj.GetUInt32();
  }
  return JSVal::Int32(static_cast<int32_t>(core::math::CLZ(x)));
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_NUMBER_H_
