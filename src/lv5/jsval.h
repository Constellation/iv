#ifndef _IV_LV5_JSVAL_H_
#define _IV_LV5_JSVAL_H_
#include <cmath>
#include <algorithm>
#include <tr1/cstdint>
#include <tr1/cmath>
#include <tr1/type_traits>
#include "byteorder.h"
#include "enable_if.h"
#include "static_assert.h"
#include "none.h"
#include "utils.h"
#include "lv5/hint.h"
#include "lv5/jsstring.h"

namespace iv {
namespace lv5 {
#ifdef IV_IS_LITTLE_ENDIAN
static const bool kLittleEndian = true;
#else
static const bool kLittleEndian = false;
#endif  // IS_LITTLE_ENDIAN

class Error;
class JSEnv;
class Context;
class JSReference;
class JSEnv;
class JSVal;
class JSObject;
class JSString;

namespace detail {
template<std::size_t PointerSize, bool IsLittle>
struct Layout {
};
template<>
struct Layout<4, true> {
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
        void* pointer_;
      } payload_;
      uint32_t tag_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 8;
};

template<>
struct Layout<8, true> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      uint32_t padding_;
      uint32_t tag_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
        void* pointer_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 16;
};

template<>
struct Layout<4, false> {
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
        void* pointer_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 8;
};

template<>
struct Layout<8, false> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      uint32_t tag_;
      uint32_t padding_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
        void* pointer_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 16;
};

struct JSTrueType { };
struct JSFalseType { };
struct JSNullType { };
struct JSUndefinedType { };
struct JSEmptyType { };
struct JSNaNType { };

union Trans64 {
  uint64_t bits_;
  double double_;
};

template<typename T>
class JSValData {
 public:
  static const Trans64 kTrans;
  static const double kNaN;
};

// 111111111111000000000000000000000000000000000000000000000000000
template<typename T>
const Trans64 JSValData<T>::kTrans = { 0x7FF8000000000000LL };

template<typename T>
const double JSValData<T>::kNaN = JSValData<T>::kTrans.double_;

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

}  // namespace iv::lv5::detail

typedef bool (*JSTrueKeywordType)(JSVal, detail::JSTrueType);
typedef bool (*JSFalseKeywordType)(JSVal, detail::JSFalseType);
typedef bool (*JSNullKeywordType)(JSVal, detail::JSNullType);
typedef bool (*JSUndefinedKeywordType)(JSVal, detail::JSUndefinedType);
typedef bool (*JSEmptyKeywordType)(JSVal, detail::JSEmptyType);
typedef bool (*JSNaNKeywordType)(JSVal, detail::JSNaNType);
inline bool JSTrue(JSVal x, detail::JSTrueType dummy);
inline bool JSFalse(JSVal x, detail::JSFalseType dummy);
inline bool JSNull(JSVal x, detail::JSNullType dummy);
inline bool JSUndefined(JSVal x, detail::JSUndefinedType dummy);
inline bool JSEmpty(JSVal x, detail::JSEmptyType dummy);
inline bool JSNaN(JSVal x, detail::JSNaNType dummy);

typedef detail::JSValData<core::None> JSValData;

class JSVal {
 public:
  typedef JSVal this_type;
  typedef detail::Layout<
            core::Size::kPointerSize,
            kLittleEndian> value_type;

  IV_STATIC_ASSERT(sizeof(value_type) == value_type::kExpectedSize);
#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3)
  IV_STATIC_ASSERT(std::tr1::is_pod<value_type>::value);
#endif

  JSVal()
    : value_() {
    set_undefined();
  }

  JSVal(const JSVal& rhs)
    : value_(rhs.value_) {
  }

  JSVal(const double& val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSObject* val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSString* val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSReference* val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSEnv* val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSTrueKeywordType val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSFalseKeywordType val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSNullKeywordType val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSUndefinedKeywordType val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSEmptyKeywordType val)  // NOLINT
    : value_() {
    set_value(val);
  }

  JSVal(JSNaNKeywordType val)  // NOLINT
    : value_() {
    set_value(val);
  }
  JSVal(const value_type& val)  // NOLINT
    : value_(val) {
  }
  template<typename T>
  JSVal(T val, typename enable_if<std::tr1::is_same<bool, T> >::type* = 0) {
    typedef std::tr1::is_same<bool, T> cond;
    IV_STATIC_ASSERT(!(cond::value));
  }

  this_type& operator=(const this_type& rhs) {
    if (this != &rhs) {
      this_type(rhs).swap(*this);
    }
    return *this;
  }

  inline void set_value(double val) {
    value_.number_.as_ = (val == val) ? val : JSValData::kNaN;
  }
  inline void set_value(JSObject* val) {
    value_.struct_.payload_.object_ = val;
    value_.struct_.tag_ = detail::kObjectTag;
  }
  inline void set_value(JSString* val) {
    value_.struct_.payload_.string_ = val;
    value_.struct_.tag_ = detail::kStringTag;
  }
  inline void set_value(JSReference* ref) {
    value_.struct_.payload_.reference_ = ref;
    value_.struct_.tag_ = detail::kReferenceTag;
  }
  inline void set_value(JSEnv* ref) {
    value_.struct_.payload_.environment_ = ref;
    value_.struct_.tag_ = detail::kEnvironmentTag;
  }
  inline void set_value(JSTrueKeywordType val) {
    value_.struct_.payload_.boolean_ = true;
    value_.struct_.tag_ = detail::kBoolTag;
  }
  inline void set_value(JSFalseKeywordType val) {
    value_.struct_.payload_.boolean_ = false;
    value_.struct_.tag_ = detail::kBoolTag;
  }
  inline void set_value(JSNullKeywordType val) {
    value_.struct_.tag_ = detail::kNullTag;
  }
  inline void set_value(JSUndefinedKeywordType val) {
    value_.struct_.tag_ = detail::kUndefinedTag;
  }
  inline void set_value(JSEmptyKeywordType val) {
    value_.struct_.tag_ = detail::kEmptyTag;
  }
  inline void set_value(JSNaNKeywordType val) {
    value_.number_.as_ = JSValData::kNaN;
  }
  inline void set_null() {
    value_.struct_.tag_ = detail::kNullTag;
  }
  inline void set_undefined() {
    value_.struct_.tag_ = detail::kUndefinedTag;
  }
  inline void set_empty() {
    value_.struct_.tag_ = detail::kEmptyTag;
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

  inline void* pointer() const {
    assert(IsPtr());
    return value_.struct_.payload_.pointer_;
  }

  inline bool IsEmpty() const {
    return value_.struct_.tag_ == detail::kEmptyTag;
  }
  inline bool IsUndefined() const {
    return value_.struct_.tag_ == detail::kUndefinedTag;
  }
  inline bool IsNull() const {
    return value_.struct_.tag_ == detail::kNullTag;
  }
  inline bool IsBoolean() const {
    return value_.struct_.tag_ == detail::kBoolTag;
  }
  inline bool IsString() const {
    return value_.struct_.tag_ == detail::kStringTag;
  }
  inline bool IsObject() const {
    return value_.struct_.tag_ == detail::kObjectTag;
  }
  inline bool IsNumber() const {
    return value_.struct_.tag_ < detail::kLowestTag;
  }
  inline bool IsReference() const {
    return value_.struct_.tag_ == detail::kReferenceTag;
  }
  inline bool IsEnvironment() const {
    return value_.struct_.tag_ == detail::kEnvironmentTag;
  }
  inline bool IsPrimitive() const {
    return IsNumber() || IsString() || IsBoolean();
  }
  inline bool IsPtr() const {
    return IsObject() || IsString() || IsReference() || IsEnvironment();
  }
  JSString* TypeOf(Context* ctx) const;

  inline uint32_t type() const {
    return IsNumber() ? detail::kNumberTag : value_.struct_.tag_;
  }

  JSObject* ToObject(Context* ctx, Error* res) const;

  JSString* ToString(Context* ctx,
                     Error* res) const;

  double ToNumber(Context* ctx, Error* res) const;

  JSVal ToPrimitive(Context* ctx,
                    Hint::Object hint, Error* res) const;

  bool IsCallable() const;

  void CheckObjectCoercible(Error* res) const;

  inline bool ToBoolean(Error* res) const {
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
      assert(!IsEmpty());
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

  static inline JSVal Bool(bool val) {
    if (val) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  }

 private:
  value_type value_;
};

inline bool JSTrue(JSVal x,
                   detail::JSTrueType dummy = detail::JSTrueType()) {
  return true;
}

inline bool JSFalse(JSVal x,
                    detail::JSFalseType dummy = detail::JSFalseType()) {
  return false;
}

inline bool JSNull(JSVal x,
                   detail::JSNullType dummy = detail::JSNullType()) {
  return false;
}

inline bool JSUndefined(
    JSVal x, detail::JSUndefinedType dummy = detail::JSUndefinedType()) {
  return false;
}

inline bool JSEmpty(JSVal x,
                    detail::JSEmptyType dummy = detail::JSEmptyType()) {
  return false;
}

inline bool JSNaN(JSVal x,
                  detail::JSNaNType dummy = detail::JSNaNType()) {
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
  assert(!lhs.IsEmpty());
  return false;
}

typedef GCVector<JSVal>::type JSVals;

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSVAL_H_
