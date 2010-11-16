#include <limits>
#include <tr1/array>
#include "jsval.h"
#include "error.h"
#include "jsobject.h"
#include "conversions.h"
namespace iv {
namespace lv5 {

JSVal::JSVal()
  : value_() {
  set_undefined();
}

JSVal::JSVal(const double& val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSObject* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSString* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSReference* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSEnv* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSTrueKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSFalseKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSNullKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSUndefinedKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSEmptyKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(const value_type& val)
  : value_(val) {
}

JSVal::JSVal(const this_type& rhs)
  : value_(rhs.value_) {
}

JSVal& JSVal::operator=(const this_type& rhs) {
  if (this != &rhs) {
    this_type(rhs).swap(*this);
  }
  return *this;
}

JSString* JSVal::TypeOf(Context* ctx) const {
  if (IsObject()) {
    if (object()->IsCallable()) {
      return JSString::NewAsciiString(ctx, "function");
    } else {
      return JSString::NewAsciiString(ctx, "object");
    }
  } else if (IsNumber()) {
    return JSString::NewAsciiString(ctx, "number");
  } else if (IsString()) {
    return JSString::NewAsciiString(ctx, "string");
  } else if (IsBoolean()) {
    return JSString::NewAsciiString(ctx, "boolean");
  } else if (IsNull()) {
    return JSString::NewAsciiString(ctx, "null");
  } else {
    assert(IsUndefined());
    return JSString::NewAsciiString(ctx, "undefined");
  }
}

JSObject* JSVal::ToObject(Context* ctx, Error* res) const {
  if (IsObject()) {
    return object();
  } else if (IsNumber()) {
    return JSNumberObject::New(ctx, number());
  } else if (IsString()) {
    return JSStringObject::New(ctx, string());
  } else if (IsBoolean()) {
    return JSBooleanObject::New(ctx, boolean());
  } else if (IsNull()) {
    res->Report(Error::Type, "null has no properties");
    return NULL;
  } else {
    assert(IsUndefined());
    res->Report(Error::Type, "undefined has no properties");
    return NULL;
  }
}

JSString* JSVal::ToString(Context* ctx,
                          Error* res) const {
  if (IsString()) {
    return string();
  } else if (IsNumber()) {
    std::tr1::array<char, 80> buffer;
    const char* const str = core::DoubleToCString(number(),
                                                  buffer.data(),
                                                  buffer.size());
    return JSString::NewAsciiString(ctx, str);
  } else if (IsBoolean()) {
    return JSString::NewAsciiString(ctx, (boolean() ? "true" : "false"));
  } else if (IsNull()) {
    return JSString::NewAsciiString(ctx, "null");
  } else if (IsUndefined()) {
    return JSString::NewAsciiString(ctx, "undefined");
  } else {
    assert(IsObject());
    JSVal prim = object()->DefaultValue(ctx, Hint::STRING, res);
    if (*res) {
      return NULL;
    }
    return prim.ToString(ctx, res);
  }
}

double JSVal::ToNumber(Context* ctx, Error* res) const {
  if (IsNumber()) {
    return number();
  } else if (IsString()) {
    return core::StringToDouble(*string(), false);
  } else if (IsBoolean()) {
    return boolean() ? 1 : +0;
  } else if (IsNull()) {
    return +0;
  } else if (IsUndefined()) {
    return std::numeric_limits<double>::quiet_NaN();
  } else {
    assert(IsObject());
    JSVal prim = object()->DefaultValue(ctx, Hint::NUMBER, res);
    if (*res) {
      return NULL;
    }
    return prim.ToNumber(ctx, res);
  }
}

JSVal JSVal::ToPrimitive(Context* ctx,
                         Hint::Object hint, Error* res) const {
  if (IsObject()) {
    return object()->DefaultValue(ctx, hint, res);
  } else {
    assert(!IsEnvironment() && !IsReference() && !IsEmpty());
    return *this;
  }
}

bool JSVal::IsCallable() const {
  return IsObject() && object()->IsCallable();
}

void JSVal::CheckObjectCoercible(Error* res) const {
  assert(!IsEnvironment() && !IsReference() && !IsEmpty());
  if (IsNull()) {
    res->Report(Error::Type, "null has no properties");
  } else if (IsUndefined()) {
    res->Report(Error::Type, "undefined has no properties");
  }
}

} }  // namespace iv::lv5
