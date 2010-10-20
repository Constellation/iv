#ifndef _IV_LV5_JSVAL_H_
#define _IV_LV5_JSVAL_H_
#include <cmath>
#include <limits>
#include <algorithm>
#include <tr1/array>
#include <tr1/cstdint>
#include <tr1/type_traits>
#include "enable_if.h"
#include "static_assert.h"
#include "utils.h"
#include "hint.h"
#include "jsstring.h"
#include "jserrorcode.h"
#include "conversions-inl.h"

namespace iv {
namespace lv5 {

class JSEnv;
class Context;
class JSReference;
class JSEnv;
class JSVal;
class JSObject;

namespace detail {
template<std::size_t PointerSize>
struct Layout {
};
#if defined(IS_LITTLE_ENDIAN)
template<>
struct Layout<4> {
  union {
    struct {
      double as_;
    } number_;
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
  };

  static const std::size_t kExpectedSize = sizeof(double);
};

template<>
struct Layout<8> {
  union {
    struct {
      uint32_t overhead_;
      double as_;
    } number_;
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
  };

  static const std::size_t kExpectedSize = 12;
};
#else
template<>
struct Layout<4> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      uint32_t tag_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = sizeof(double);
};

template<>
struct Layout<8> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      uint32_t tag_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 12;
};
#endif  // define(IS_LITTLE_ENDIAN)

struct JSTrueType { };
struct JSFalseType { };
struct JSNullType { };
struct JSUndefinedType { };

}  // namespace iv::lv5::detail

typedef bool (*JSTrueKeywordType)(JSVal, detail::JSTrueType);
typedef bool (*JSFalseKeywordType)(JSVal, detail::JSFalseType);
typedef bool (*JSNullKeywordType)(JSVal, detail::JSNullType);
typedef bool (*JSUndefinedKeywordType)(JSVal, detail::JSUndefinedType);
inline bool JSTrue(JSVal x, detail::JSTrueType dummy);
inline bool JSFalse(JSVal x, detail::JSFalseType dummy);
inline bool JSNull(JSVal x, detail::JSNullType dummy);
inline bool JSUndefined(JSVal x, detail::JSUndefinedType dummy);

class JSVal {
 public:
  typedef JSVal this_type;
  typedef detail::Layout<core::Size::kPointerSize> value_type;

  IV_STATIC_ASSERT(sizeof(value_type) == value_type::kExpectedSize);
  IV_STATIC_ASSERT(std::tr1::is_pod<value_type>::value);

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
  JSVal(const double& val);  // NOLINT
  JSVal(JSObject* val);  // NOLINT
  JSVal(JSString* val);  // NOLINT
  JSVal(JSReference* val);  // NOLINT
  JSVal(JSEnv* val);  // NOLINT
  JSVal(JSTrueKeywordType val);  // NOLINT
  JSVal(JSFalseKeywordType val);  // NOLINT
  JSVal(JSNullKeywordType val);  // NOLINT
  JSVal(JSUndefinedKeywordType val);  // NOLINT
  JSVal(const value_type& val);  // NOLINT
  template<typename T>
  JSVal(T val, typename enable_if<std::tr1::is_same<bool, T> >::type* = 0) {
    typedef std::tr1::is_same<bool, T> cond;
    IV_STATIC_ASSERT(!(cond::value));
  }

  this_type& operator=(const this_type&);

  inline void set_value(double val) {
    value_.number_.as_ = val;
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
  inline void set_value(JSTrueKeywordType val) {
    value_.struct_.payload_.boolean_ = true;
    value_.struct_.tag_ = kBoolTag;
  }
  inline void set_value(JSFalseKeywordType val) {
    value_.struct_.payload_.boolean_ = false;
    value_.struct_.tag_ = kBoolTag;
  }
  inline void set_value(JSNullKeywordType val) {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kNullTag;
  }
  inline void set_value(JSUndefinedKeywordType val) {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kUndefinedTag;
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
    return value_.number_.as_;
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
    return IsNumber() || IsString() || IsBoolean();
  }
  JSString* TypeOf(Context* ctx) const;

  inline uint32_t type() const {
    return IsNumber() ? kNumberTag : value_.struct_.tag_;
  }

  JSObject* ToObject(Context* ctx, JSErrorCode::Type* res) const;

  JSString* ToString(Context* ctx,
                     JSErrorCode::Type* res) const;

  double ToNumber(Context* ctx, JSErrorCode::Type* res) const;

  JSVal ToPrimitive(Context* ctx,
                    Hint::Object hint, JSErrorCode::Type* res) const;

  bool IsCallable() const;

  inline void CheckObjectCoercible(JSErrorCode::Type* res) const {
    assert(!IsEnvironment() && !IsReference());
    if (IsNull() || IsUndefined()) {
      *res = JSErrorCode::TypeError;
    }
  }

  inline bool ToBoolean(JSErrorCode::Type* res) const {
    if (IsNumber()) {
      const double& num = number();
      return num != 0 && !std::isnan(num);
    } else if (IsString()) {
      return !string()->empty();
    } else if (IsNull()) {
      return false;
    } else if (IsUndefined()) {
      return false;
    } else if (IsBoolean()) {
      return boolean();
    } else {
      return true;
    }
  }

  inline const value_type& Layout() const {
    return value_;
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(value_, rhs.value_);
  }

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }


 private:
   value_type value_;
};

inline bool JSTrue(JSVal x, detail::JSTrueType dummy = detail::JSTrueType()) {
  return true;
}

inline bool JSFalse(JSVal x, detail::JSFalseType dummy = detail::JSFalseType()) {
  return false;
}

inline bool JSNull(JSVal x, detail::JSNullType dummy = detail::JSNullType()) {
  return false;
}

inline bool JSUndefined(JSVal x, detail::JSUndefinedType dummy = detail::JSUndefinedType()) {
  return false;
}

inline bool SameValue(const JSVal& lhs, const JSVal& rhs) {
  if (lhs.type() != rhs.type()) {
    return false;
  }
  if (lhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber()) {
    const double& lhsv = lhs.number();
    const double& rhsv = rhs.number();
    if (std::isnan(lhsv) && std::isnan(rhsv)) {
      return true;
    }
    if (lhsv == rhsv) {
      if (std::signbit(lhsv) && std::signbit(rhsv)) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  if (lhs.IsString()) {
    return *(lhs.string()) == *(rhs.string());
  }
  if (lhs.IsBoolean()) {
    return lhs.boolean() == rhs.boolean();
  }
  if (lhs.IsObject()) {
    return lhs.object() == rhs.object();
  }
  return false;
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSVAL_H_
