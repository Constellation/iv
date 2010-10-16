#ifndef _IV_LV5_JSVAL_H_
#define _IV_LV5_JSVAL_H_
#include <cmath>
#include <limits>
#include <algorithm>
#include <boost/utility/enable_if.hpp>
#include <iostream>  // NOLINT
#include <tr1/array>
#include <tr1/cstdint>
#include "utils.h"
#include "jsstring.h"
#include "jsobject.h"
#include "jserrorcode.h"
#include "conversions-inl.h"

namespace iv {
namespace lv5 {
class JSEnv;
class Context;

#define SPECIALIZE(TYPE)\
template <Tag V>\
struct vtable_initializer<V, typename boost::enable_if_c<V == TYPE>::type>

#define VPTR(TYPE)\
(vtables + TYPE)

// &vtable_initializer<TYPE>::vtable_

class JSReference;
class JSEnv;

struct Null {
};

class JSVal {
 public:
  typedef JSVal this_type;

  enum Tag {
    NUMBER = 0,
    OBJECT,
    STRING,
    BOOLEAN,
    NULLVALUE,
    UNDEFINED,
    REFERENCE,
    ENVIRONMENT
  };

  static const uint32_t kTrueTag        = 0xffffffff;
  static const uint32_t kFalseTag       = 0xfffffffe;
  static const uint32_t kEmptyTag       = 0xfffffffd;
  static const uint32_t kEnvironmentTag = 0xfffffffc;
  static const uint32_t kReferenceTag   = 0xfffffffb;
  static const uint32_t kUndefinedTag   = 0xfffffffa;
  static const uint32_t kNullTag        = 0xfffffff9;
  static const uint32_t kBoolTag        = 0xfffffff8;
  static const uint32_t kStringTag      = 0xfffffff7;
  static const uint32_t kObjectTag      = 0xfffffff6;
  static const uint32_t kNumberTag      = 0xfffffff5;
  static const uint32_t kHighestTag     = kTrueTag;
  static const uint32_t kLowestTag      = kNumberTag;
  static const uint32_t kVtables        = kHighestTag - kLowestTag + 1;

  JSVal();
  JSVal(const JSVal& rhs);
  explicit JSVal(const double& val);
  explicit JSVal(JSObject* val);
  explicit JSVal(JSString* val);
  explicit JSVal(JSReference* val);
  explicit JSVal(JSEnv* val);
  explicit JSVal(bool val);

  this_type& operator=(const this_type&);

  inline void set_value(double val) {
    value_.number_ = val;
  }
  inline void set_value(JSObject* val) {
    value_.struct_.payload_.object_ = val;
    value_.struct_.tag_ = kObjectTag;
  }
  inline void set_value(JSString* val) {
    value_.struct_.payload_.string_ = val;
    value_.struct_.tag_ = kStringTag;
  }
  inline void set_value(JSReference* ref) {
    value_.struct_.payload_.reference_ = ref;
    value_.struct_.tag_ = kReferenceTag;
  }
  inline void set_value(JSEnv* ref) {
    value_.struct_.payload_.environment_ = ref;
    value_.struct_.tag_ = kEnvironmentTag;
  }
  inline void set_value(bool val) {
    value_.struct_.payload_.boolean_ = val;
    value_.struct_.tag_ = kBoolTag;
  }
  inline void set_null() {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kNullTag;
  }
  inline void set_undefined() {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kUndefinedTag;
  }

  inline JSReference* reference() const {
    assert(IsReference());
    return value_.struct_.payload_.reference_;
  }

  inline JSEnv* environment() const {
    assert(IsEnvironment());
    return value_.struct_.payload_.environment_;
  }

  inline JSString* string() const {
    assert(IsString());
    return value_.struct_.payload_.string_;
  }

  inline JSObject* object() const {
    assert(IsObject());
    return value_.struct_.payload_.object_;
  }

  inline const double& number() const {
    assert(IsNumber());
    return value_.number_;
  }

  inline bool boolean() const {
    assert(IsBoolean());
    return value_.struct_.payload_.boolean_;
  }

  inline bool IsUndefined() const {
    return value_.struct_.tag_ == kUndefinedTag;
  }
  inline bool IsNull() const {
    return value_.struct_.tag_ == kNullTag;
  }
  inline bool IsBoolean() const {
    return value_.struct_.tag_ == kBoolTag;
  }
  inline bool IsString() const {
    return value_.struct_.tag_ == kStringTag;
  }
  inline bool IsObject() const {
    return value_.struct_.tag_ == kObjectTag;
  }
  inline bool IsNumber() const {
    return value_.struct_.tag_ < kLowestTag;
  }
  inline bool IsReference() const {
    return value_.struct_.tag_ == kReferenceTag;
  }
  inline bool IsEnvironment() const {
    return value_.struct_.tag_ == kEnvironmentTag;
  }
  inline bool IsPrimitive() const {
    if (IsNumber()) {
      return true;
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->IsPrimitive(this);
    }
  }
  inline JSString* TypeOf(Context* ctx) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->TypeOf(this, ctx);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->TypeOf(this, ctx);
    }
  }
  inline uint32_t type() const {
    if (IsNumber()) {
      return kNumberTag;
    } else {
      return value_.struct_.tag_;
    }
  }

  inline JSObject* ToObject(Context* ctx, JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->ToObject(this, ctx, res);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->ToObject(this, ctx, res);
    }
  }

  inline JSString* ToString(Context* ctx,
                            JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->ToString(this, ctx, res);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->ToString(this, ctx, res);
    }
  }

  inline double ToNumber(Context* ctx, JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->ToNumber(this, ctx, res);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->ToNumber(this, ctx, res);
    }
  }

  inline JSVal ToPrimitive(Context* ctx,
                           JSObject::Hint hint, JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->ToPrimitive(this, ctx, hint, res);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->ToPrimitive(this, ctx, hint, res);
    }
  }

  inline void CheckObjectCoercible(JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->CheckObjectCoercible(this, res);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->CheckObjectCoercible(this, res);
    }
  }

  inline bool IsCallable() const {
    if (IsNumber()) {
      return false;
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->IsCallable(this);
    }
  }

  inline bool ToBoolean(JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return VPTR(NUMBER)->ToBoolean(this, res);
    } else {
      return VPTR(value_.struct_.tag_ - kLowestTag)->ToBoolean(this, res);
    }
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(value_, rhs.value_);
  }

  inline friend void swap(this_type& lhs, this_type& rhs) {
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
    JSVal::Tag value;
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

  static vtable vtables[kVtables];

  template <Tag T, typename U = void>
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

  union JSValImpl {
    double number_;
    struct {
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
      uint32_t tag_;
    } struct_;
  } value_;
};

#undef SPECIALIZE
#undef VPTR
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSVAL_H_
