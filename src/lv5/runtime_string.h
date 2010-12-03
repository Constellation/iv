#ifndef _IV_LV5_RUNTIME_STRING_H_
#define _IV_LV5_RUNTIME_STRING_H_
#include <vector>
#include "ustring.h"
#include "ustringpiece.h"
#include "conversions.h"
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsstring.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

static inline JSVal StringToStringValueOfImpl(const Arguments& args,
                                              Error* error,
                                              const char* msg) {
  const JSVal& obj = args.this_binding();
  if (!obj.IsString()) {
    if (obj.IsObject() && obj.object()->AsStringObject()) {
      return obj.object()->AsStringObject()->value();
    } else {
      error->Report(Error::Type, msg);
      return JSUndefined;
    }
  } else {
    return obj.string();
  }
}

}  // namespace iv::lv5::runtime::detail

// section 15.5.1
inline JSVal StringConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    JSString* str;
    if (args.size() > 0) {
      str = args[0].ToString(args.ctx(), ERROR(error));
    } else {
      str = JSString::NewEmptyString(args.ctx());
    }
    return JSStringObject::New(args.ctx(), str);
  } else {
    if (args.size() > 0) {
      return args[0].ToString(args.ctx(), error);
    } else {
      return JSString::NewEmptyString(args.ctx());
    }
  }
}

// section 15.5.3.2 String.fromCharCode([char0 [, char1[, ...]]])
inline JSVal StringFromCharCode(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.fromCharCode", args, error);
  std::vector<uc16> buf(args.size());
  std::vector<uc16>::iterator target = buf.begin();
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it, ++target) {
    const double val = it->ToNumber(args.ctx(), ERROR(error));
    *target = core::DoubleToUInt32(val);
  }
  return JSString::New(args.ctx(), core::UStringPiece(buf.data(), buf.size()));
}

// section 15.5.4.2 String.prototype.toString()
inline JSVal StringToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toString", args, error);
  return detail::StringToStringValueOfImpl(
      args, error,
      "String.prototype.toString is not generic function");
}

// section 15.5.4.3 String.prototype.valueOf()
inline JSVal StringValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.valueOf", args, error);
  return detail::StringToStringValueOfImpl(
      args, error,
      "String.prototype.valueOf is not generic function");
}

// section 15.5.4.4 String.prototype.charAt(pos)
inline JSVal StringCharAt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.charAt", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  double position;
  if (args.size() > 0) {
    position = args[0].ToNumber(args.ctx(), ERROR(error));
    position = core::DoubleToInteger(position);
  } else {
    // undefined -> NaN -> 0
    position = 0;
  }
  if (position < 0 || position >= str->size()) {
    return JSString::NewEmptyString(args.ctx());
  } else {
    return JSString::New(
        args.ctx(),
        core::UStringPiece(str->data() + core::DoubleToUInt32(position), 1));
  }
}

// section 15.5.4.5 String.prototype.charCodeAt(pos)
inline JSVal StringCharCodeAt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.charCodeAt", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  double position;
  if (args.size() > 0) {
    position = args[0].ToNumber(args.ctx(), ERROR(error));
    position = core::DoubleToInteger(position);
  } else {
    // undefined -> NaN -> 0
    position = 0;
  }
  if (position < 0 || position >= str->size()) {
    return JSNaN;
  } else {
    return (*str)[core::DoubleToUInt32(position)];
  }
}

// section 15.5.4.6 String.prototype.concat([string1[, string2[, ...]]])
inline JSVal StringConcat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.concat", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  core::UString result(str->begin(), str->end());
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const JSString* const r = it->ToString(args.ctx(), ERROR(error));
    result.append(r->begin(), r->end());
  }
  return JSString::New(args.ctx(), result);
}

// section 15.5.4.7 String.prototype.indexOf(searchString, position)
inline JSVal StringIndexOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.indexOf", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* search_str;
  // undefined -> NaN -> 0
  double position = 0;
  if (args.size() > 0) {
    search_str = args[0].ToString(args.ctx(), ERROR(error));
    if (args.size() > 1) {
      position = args[1].ToNumber(args.ctx(), ERROR(error));
      position = core::DoubleToInteger(position);
    }
  } else {
    // undefined -> "undefined"
    search_str = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  const std::size_t start = std::min(
      static_cast<std::size_t>(std::max(position, 0.0)), str->size());
  const GCUString::size_type loc =
      str->value().find(search_str->value(), start);
  return (loc == GCUString::npos) ? -1.0 : loc;
}

// section 15.5.4.8 String.prototype.lastIndexOf(searchString, position)
inline JSVal StringLastIndexOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.lastIndexOf", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* search_str;
  // undefined -> NaN -> 0
  std::size_t pos = std::numeric_limits<std::size_t>::max();
  if (args.size() > 0) {
    search_str = args[0].ToString(args.ctx(), ERROR(error));
    if (args.size() > 1) {
      const double position = args[1].ToNumber(args.ctx(), ERROR(error));
      pos = core::DoubleToUInt32(core::DoubleToInteger(position));
    }
  } else {
    // undefined -> "undefined"
    search_str = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  const GCUString::size_type loc =
      str->value().rfind(search_str->value(), std::min(pos, str->size()));
  return (loc == GCUString::npos) ? -1.0 : loc;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_STRING_H_
