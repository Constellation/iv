#ifndef _IV_LV5_JSVAL_H_
#define _IV_LV5_JSVAL_H_
#include <inttypes.h>
#include <cmath>
#include <limits>
#include <algorithm>
#include <boost/utility/enable_if.hpp>
#include <iostream>  // NOLINT
#include <tr1/array>
#include "jsstring.h"
#include "jsobject.h"
#include "jserrorcode.h"
#include "conversions-inl.h"

namespace iv {
namespace lv5 {

class JSEnv;
class Context;
#define SET_VTABLE(TYPE)\
template <JSVal::Type V>\
JSVal::vtable JSVal::vtable_initializer< \
  V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::vtable_ = { \
    JSVal:: TYPE, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::IsPrimitive, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::TypeOf, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::ToObject, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::ToBoolean, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::ToString, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::ToNumber , \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::ToPrimitive , \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c< \
          V == JSVal:: TYPE>::type>::CheckObjectCoercible, \
    &JSVal::vtable_initializer< \
      V, typename boost::enable_if_c<V == JSVal:: TYPE>::type>::IsCallable\
}

#define SPECIALIZE(TYPE)\
template <Type V>\
struct vtable_initializer<V, typename boost::enable_if_c<V == TYPE>::type>

#define INIT_VTABLE(TYPE)\
vptr_ = VPTR(TYPE);

#define VPTR(TYPE)\
&vtable_initializer<TYPE>::vtable_

class JSReference;
class JSEnv;

class JSVal {
 public:
  typedef JSVal this_type;
  enum Type {
    NUMBER,
    OBJECT,
    STRING,
    BOOLEAN,
    NULLVALUE,
    UNDEFINED,
    REFERENCE,
    ENVIRONMENT
  };
  JSVal();
  JSVal(const JSVal& rhs);
  explicit JSVal(const double& val);
  explicit JSVal(JSObject* val);
  explicit JSVal(JSString* val);
  explicit JSVal(JSReference* val);
  explicit JSVal(JSEnv* val);
  explicit JSVal(bool val);

  this_type& operator=(const this_type&);

  void set_value(double val) {
    value_.number_ = val;
    INIT_VTABLE(NUMBER);
  }
  void set_value(JSObject* val) {
    value_.object_ = val;
    INIT_VTABLE(OBJECT);
  }
  void set_value(JSString* val) {
    value_.string_ = val;
    INIT_VTABLE(STRING);
  }
  void set_value(JSReference* ref) {
    value_.reference_ = ref;
    INIT_VTABLE(REFERENCE);
  }
  void set_value(JSEnv* ref) {
    value_.environment_ = ref;
    INIT_VTABLE(ENVIRONMENT);
  }
  void set_value(bool val) {
    value_.boolean_ = val;
    INIT_VTABLE(BOOLEAN);
  }
  void set_null() {
    value_.boolean_ = NULL;
    INIT_VTABLE(NULLVALUE);
  }
  void set_undefined() {
    value_.boolean_ = NULL;
    INIT_VTABLE(UNDEFINED);
  }

  JSReference* reference() const {
    return value_.reference_;
  }

  JSEnv* environment() const {
    return value_.environment_;
  }

  JSString* string() const {
    return value_.string_;
  }

  JSObject* object() const {
    return value_.object_;
  }

  const double& number() const {
    return value_.number_;
  }

  bool boolean() const {
    return value_.boolean_;
  }

  bool IsUndefined() const {
    return vptr_ == VPTR(UNDEFINED);
  }
  bool IsNull() const {
    return vptr_ == VPTR(NULLVALUE);
  }
  bool IsBoolean() const {
    return vptr_ == VPTR(BOOLEAN);
  }
  bool IsString() const {
    return vptr_ == VPTR(STRING);
  }
  bool IsObject() const {
    return vptr_ == VPTR(OBJECT);
  }
  bool IsNumber() const {
    return vptr_ == VPTR(NUMBER);
  }
  bool IsReference() const {
    return vptr_ == VPTR(REFERENCE);
  }
  bool IsEnvironment() const {
    return vptr_ == VPTR(ENVIRONMENT);
  }
  bool IsPrimitive() const {
    return vptr_->IsPrimitive(this);
  }
  JSString* TypeOf(Context* context) const {
    return vptr_->TypeOf(this, context);
  }
  Type type() const {
    return vptr_->value;
  }

  JSObject* ToObject(Context* ctx, JSErrorCode::Type* res) const {
    return vptr_->ToObject(this, ctx, res);
  }

  JSString* ToString(Context* context,
                     JSErrorCode::Type* res) const {
    return vptr_->ToString(this, context, res);
  }

  double ToNumber(Context* context, JSErrorCode::Type* res) const {
    return vptr_->ToNumber(this, context, res);
  }

  JSVal ToPrimitive(Context* context,
                    JSObject::Hint hint, JSErrorCode::Type* res) const {
    return vptr_->ToPrimitive(this, context, hint, res);
  }

  void CheckObjectCoercible(JSErrorCode::Type* res) const {
    vptr_->CheckObjectCoercible(this, res);
  }

  bool IsCallable() const {
    return vptr_->IsCallable(this);
  }

  bool ToBoolean(JSErrorCode::Type* res) const {
    return vptr_->ToBoolean(this, res);
  }

  void swap(this_type& rhs) {
    using std::swap;
    swap(vptr_, rhs.vptr_);
    swap(value_, rhs.value_);
  }

  friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

  static JSVal Undefined();
  static JSVal Null();
  static JSVal Boolean(bool val);
  static JSVal Number(double val);
  static JSVal String(JSString* str);
  static JSVal Object(JSObject* obj);

 private:

  struct vtable {
    JSVal::Type value;
    bool (*IsPrimitive)(const JSVal* self);
    JSString* (*TypeOf)(const JSVal* self, Context* context);
    JSObject* (*ToObject)(const JSVal* self, Context* ctx,
                          JSErrorCode::Type* res);
    bool (*ToBoolean)(const JSVal* self, JSErrorCode::Type* res);
    JSString* (*ToString)(const JSVal* self,
                          Context* context, JSErrorCode::Type* res);
    double (*ToNumber)(const JSVal* self, Context* context,
                       JSErrorCode::Type* res);
    JSVal (*ToPrimitive)(const JSVal* self, Context* context,
                         JSObject::Hint hint, JSErrorCode::Type* res);
    void (*CheckObjectCoercible)(const JSVal* self, JSErrorCode::Type* res);
    bool (*IsCallable)(const JSVal* self);
  };

  template <Type T, typename U = void>
  struct vtable_initializer {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self);
    static JSString* TypeOf(const JSVal* self, Context* context);
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type* res);
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type* res);
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res);
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res);
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res);
    static void CheckObjectCoercible(const JSVal* self, JSErrorCode::Type* res);
    static bool IsCallable(const JSVal* self);
  };

  SPECIALIZE(OBJECT) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return false;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      if (self->object()->IsCallable()) {
        return JSString::NewAsciiString(context, "function");
      } else {
        return JSString::NewAsciiString(context, "object");
      }
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type*) {
      return self->object();
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type*) {
      return true;
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      JSVal prim = ToPrimitive(self, context, JSObject::STRING, res);
      if (*res) {
        return NULL;
      }
      return prim.ToString(context, res);
    }
    static double ToNumber(const JSVal* self,
                           Context* context, JSErrorCode::Type* res) {
      JSVal prim = ToPrimitive(self, context, JSObject::NUMBER, res);
      if (*res) {
        return NULL;
      }
      return prim.ToNumber(context, res);
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      return self->object()->DefaultValue(context, hint, res);
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return self->object()->IsCallable();
    }
  };
  SPECIALIZE(BOOLEAN) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return true;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "boolean");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type*) {
      return JSBooleanObject::New(ctx, self->boolean());
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type*) {
      return self->boolean();
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      if (self->boolean()) {
        return JSString::NewAsciiString(context, "true");
      } else {
        return JSString::NewAsciiString(context, "false");
      }
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      return self->boolean() ? 1 : +0;
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      return *self;
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };
  SPECIALIZE(STRING) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return true;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "string");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type*) {
      return JSStringObject::New(ctx, self->string());
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type*) {
      return !self->string()->empty();
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      return self->string();
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      return core::StringToDouble(*self->string());
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      return *self;
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };
  SPECIALIZE(NUMBER) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return true;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "number");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type*) {
      return JSNumberObject::New(ctx, self->number());
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type*) {
      const double& num = self->number();
      return num != 0 && !std::isnan(num);
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      std::tr1::array<char, 80> buffer;
      const char* const str = core::DoubleToCString(self->number(),
                                                    buffer.data(),
                                                    buffer.size());
      return JSString::NewAsciiString(context, str);
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      return self->number();
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      return *self;
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };
  SPECIALIZE(NULLVALUE) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return false;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "null");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return NULL;
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type*) {
      return false;
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      return JSString::NewAsciiString(context, "null");
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      return +0;
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      return *self;
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };
  SPECIALIZE(UNDEFINED) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return false;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "undefined");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return NULL;
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type*) {
      return false;
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      return JSString::NewAsciiString(context, "undefined");
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      return *self;
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };
  SPECIALIZE(REFERENCE) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return false;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "reference");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return NULL;
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return false;
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return NULL;
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return 0;
    }
    static JSVal ToPrimitive(const JSVal* self, Context* context,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return JSVal::Undefined();
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };
  SPECIALIZE(ENVIRONMENT) {
    static vtable vtable_;
    static bool IsPrimitive(const JSVal* self) {
      return false;
    }
    static JSString* TypeOf(const JSVal* self, Context* context) {
      return JSString::NewAsciiString(context, "environment");
    }
    static JSObject* ToObject(const JSVal* self, Context* ctx,
                              JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return NULL;
    }
    static bool ToBoolean(const JSVal* self, JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return false;
    }
    static JSString* ToString(const JSVal* self,
                              Context* context, JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return NULL;
    }
    static double ToNumber(const JSVal* self, Context* context,
                           JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return 0;
    }
    static JSVal ToPrimitive(const JSVal* self, Context* contex,
                             JSObject::Hint hint, JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return JSVal::Undefined();
    }
    static void CheckObjectCoercible(const JSVal* self,
                                     JSErrorCode::Type* res) {
      *res = JSErrorCode::TypeError;
      return;
    }
    static bool IsCallable(const JSVal* self) {
      return false;
    }
  };

  vtable* vptr_;
  union JSValImpl {
    double number_;
    bool boolean_;
    JSObject* object_;
    JSString* string_;
    JSReference* reference_;
    JSEnv* environment_;
  } value_;
};

SET_VTABLE(OBJECT);
SET_VTABLE(STRING);
SET_VTABLE(BOOLEAN);
SET_VTABLE(NUMBER);
SET_VTABLE(NULLVALUE);
SET_VTABLE(UNDEFINED);
SET_VTABLE(REFERENCE);
SET_VTABLE(ENVIRONMENT);

#undef SET_VTABLE
#undef SPECIALIZE
#undef INIT_VTABLE
#undef VPTR
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSVAL_H_
