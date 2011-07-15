#include <limits>
#include "detail/array.h"
#include "dtoa.h"
#include "lv5/error_check.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/jsbooleanobject.h"
#include "lv5/jsnumberobject.h"
#include "lv5/jsstringobject.h"
namespace iv {
namespace lv5 {

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
    return JSString::NewAsciiString(ctx, "object");
  } else {
    assert(IsUndefined());
    return JSString::NewAsciiString(ctx, "undefined");
  }
}

JSObject* JSVal::ToObject(Context* ctx, Error* e) const {
  if (IsObject()) {
    return object();
  } else if (IsNumber()) {
    return JSNumberObject::New(ctx, number());
  } else if (IsString()) {
    return JSStringObject::New(ctx, string());
  } else if (IsBoolean()) {
    return JSBooleanObject::New(ctx, boolean());
  } else if (IsNull()) {
    e->Report(Error::Type, "null has no properties");
    return NULL;
  } else {
    assert(IsUndefined());
    e->Report(Error::Type, "undefined has no properties");
    return NULL;
  }
}

JSString* JSVal::ToString(Context* ctx, Error* e) const {
  if (IsString()) {
    return string();
  } else if (IsNumber()) {
    std::array<char, 80> buffer;
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
    JSVal prim = object()->DefaultValue(ctx, Hint::STRING,
                                        IV_LV5_ERROR_WITH(e, NULL));
    return prim.ToString(ctx, e);
  }
}

double JSVal::ToNumber(Context* ctx, Error* e) const {
  if (IsNumber()) {
    return number();
  } else if (IsString()) {
    return core::StringToDouble(*string()->Flatten(), false);
  } else if (IsBoolean()) {
    return boolean() ? 1 : +0;
  } else if (IsNull()) {
    return +0;
  } else if (IsUndefined()) {
    return core::kNaN;
  } else {
    assert(IsObject());
    JSVal prim = object()->DefaultValue(ctx, Hint::NUMBER,
                                        IV_LV5_ERROR_WITH(e, 0.0));
    return prim.ToNumber(ctx, e);
  }
}

JSVal JSVal::ToPrimitive(Context* ctx, Hint::Object hint, Error* e) const {
  if (IsObject()) {
    return object()->DefaultValue(ctx, hint, e);
  } else {
    assert(!IsEnvironment() && !IsReference() && !IsEmpty());
    return *this;
  }
}

bool JSVal::IsCallable() const {
  return IsObject() && object()->IsCallable();
}

void JSVal::CheckObjectCoercible(Error* e) const {
  assert(!IsEnvironment() && !IsReference() && !IsEmpty());
  if (IsNull()) {
    e->Report(Error::Type, "null has no properties");
  } else if (IsUndefined()) {
    e->Report(Error::Type, "undefined has no properties");
  }
}

} }  // namespace iv::lv5
