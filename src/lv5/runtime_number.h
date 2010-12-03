#ifndef _IV_LV5_RUNTIME_NUMBER_H_
#define _IV_LV5_RUNTIME_NUMBER_H_
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
    if (obj.IsObject() && obj.object()->AsNumberObject()) {
      num = obj.object()->AsNumberObject()->value();
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
    double radix = first.ToNumber(args.ctx(), ERROR(error));
    radix = core::DoubleToInteger(radix);
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
        JSString* const result = JSString::NewEmptyString(args.ctx());
        core::DoubleToStringWithRadix(
            num,
            static_cast<int>(radix),
            result);
        result->ReCalcHash();
        return result;
      }
    } else {
      // TODO(Constellation) more details
      error->Report(Error::Range,
                    "inllegal radix");
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

// section 15.7.4.4 Number.prototype.valueOf()
inline JSVal NumberValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Number.prototype.valueOf", args, error);
  const JSVal& obj = args.this_binding();
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->AsNumberObject()) {
      return obj.object()->AsNumberObject()->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.valueOf is not generic function");
      return JSUndefined;
    }
  } else {
    return obj.number();
  }
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_NUMBER_H_
