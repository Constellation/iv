#ifndef IV_LV5_JSVAL_H_
#define IV_LV5_JSVAL_H_
#include <cmath>
#include <algorithm>
#include "detail/cstdint.h"
#include "detail/type_traits.h"
#include "byteorder.h"
#include "gc_template.h"
#include "enable_if.h"
#include "static_assert.h"
#include "platform_math.h"
#include "canonicalized_nan.h"
#include "conversions.h"
#include "none.h"
#include "utils.h"
#include "lv5/hint.h"
#include "lv5/jsstring_fwd.h"

namespace iv {
namespace lv5 {

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
struct Layout;

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
        JSVal* jsvalref_;
        int32_t int32_;
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
        JSVal* jsvalref_;
        int32_t int32_;
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
        JSVal* jsvalref_;
        int32_t int32_;
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
        JSVal* jsvalref_;
        int32_t int32_;
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

static const uint32_t kOtherPtrTag    = 0xffffffff;  // ptr range end
static const uint32_t kEnvironmentTag = 0xfffffffe;
static const uint32_t kReferenceTag   = 0xfffffffd;
static const uint32_t kStringTag      = 0xfffffffc;
static const uint32_t kObjectTag      = 0xfffffffb;  // ptr range start
static const uint32_t kEmptyTag       = 0xfffffffa;
static const uint32_t kUndefinedTag   = 0xfffffff9;
static const uint32_t kNullTag        = 0xfffffff8;
static const uint32_t kBoolTag        = 0xfffffff7;
static const uint32_t kNumberTag      = 0xfffffff5;
static const uint32_t kInt32Tag       = 0xfffffff4;

inline bool InPtrRange(uint32_t tag) {
  return kObjectTag <= tag;
}

struct Int32Tag { };
struct UInt32Tag { };
struct UInt16Tag { };
struct OtherPtrTag { };

}  // namespace detail

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

class JSVal {
 public:
  typedef JSVal this_type;
  typedef detail::Layout<
            core::Size::kPointerSize,
            core::kLittleEndian> value_type;

  IV_STATIC_ASSERT(sizeof(value_type) == value_type::kExpectedSize);
#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3)
  IV_STATIC_ASSERT(std::is_pod<value_type>::value);
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
  JSVal(T val, typename enable_if<std::is_same<bool, T> >::type* = 0) {
    typedef std::is_same<bool, T> cond;
    IV_STATIC_ASSERT(!(cond::value));
  }

  this_type& operator=(const this_type& rhs) {
    if (this != &rhs) {
      this_type(rhs).swap(*this);
    }
    return *this;
  }

  inline void set_value(double val) {
    const int32_t i = static_cast<int32_t>(val);
    if (val != i || (!i && core::Signbit(val))) {
      // this value is not represented by int32_t
      value_.number_.as_ = (val == val) ? val : core::kNaN;
    } else {
      set_value_int32(i);
    }
  }

  template<typename T>
  inline void set_value_ptr(T* ptr) {
    value_.struct_.payload_.pointer_ = reinterpret_cast<void*>(ptr);
    value_.struct_.tag_ = detail::kOtherPtrTag;
  }

  inline void set_value_int32(int32_t val) {
    value_.struct_.payload_.int32_ = val;
    value_.struct_.tag_ = detail::kInt32Tag;
  }

  inline void set_value_uint32(uint32_t val) {
    if (static_cast<int32_t>(val) < 0) {  // LSB is 1
      value_.number_.as_ = val;
    } else {
      set_value_int32(static_cast<int32_t>(val));
    }
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
    value_.number_.as_ = core::kNaN;
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

  inline double number() const {
    assert(IsNumber());
    if (IsInt32()) {
      return int32();
    } else {
      return value_.number_.as_;
    }
  }

  inline int32_t int32() const {
    assert(IsInt32());
    return value_.struct_.payload_.int32_;
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

  inline bool IsInt32() const {
    return value_.struct_.tag_ == detail::kInt32Tag;
  }

  inline bool IsNumber() const {
    return value_.struct_.tag_ < detail::kNumberTag;
  }

  inline bool IsReference() const {
    return value_.struct_.tag_ == detail::kReferenceTag;
  }

  inline bool IsEnvironment() const {
    return value_.struct_.tag_ == detail::kEnvironmentTag;
  }

  inline bool IsOtherPtr() const {
    return value_.struct_.tag_ == detail::kOtherPtrTag;
  }

  inline bool IsPrimitive() const {
    return IsNumber() || IsString() || IsBoolean();
  }

  inline bool IsPtr() const {
    return detail::InPtrRange(value_.struct_.tag_);
  }

  JSString* TypeOf(Context* ctx) const;

  inline uint32_t type() const {
    return IsNumber() ? detail::kNumberTag : value_.struct_.tag_;
  }

  JSObject* GetPrimitiveProto(Context* ctx) const;

  JSObject* ToObject(Context* ctx, Error* e) const;

  JSString* ToString(Context* ctx, Error* e) const;

  double ToNumber(Context* ctx, Error* e) const;

  int32_t ToInt32(Context* ctx, Error* e) const {
    if (IsInt32()) {
      return int32();
    } else {
      return core::DoubleToInt32(ToNumber(ctx, e));
    }
  }

  uint32_t ToUInt32(Context* ctx, Error* e) const {
    if (IsInt32() && int32() >= 0) {
      return static_cast<uint32_t>(int32());
    } else {
      return core::DoubleToUInt32(ToNumber(ctx, e));
    }
  }

  uint32_t GetUInt32() const {
    assert(IsNumber());
    uint32_t val = 0;  // make gcc happy
    GetUInt32(&val);
    return val;
  }

  bool GetUInt32(uint32_t* result) const {
    if (IsInt32() && int32() >= 0) {
      *result = static_cast<uint32_t>(int32());
      return true;
    } else if (IsNumber()) {
      const double val = number();
      const uint32_t res = static_cast<uint32_t>(val);
      if (val == res) {
        *result = res;
        return true;
      }
    }
    return false;
  }

  JSVal ToPrimitive(Context* ctx, Hint::Object hint, Error* e) const;

  bool IsCallable() const;

  void CheckObjectCoercible(Error* e) const;

  inline bool ToBoolean(Error* e) const {
    if (IsNumber()) {
      const double num = number();
      return num != 0 && !core::IsNaN(num);
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

  template<typename T>
  static inline JSVal UInt32(
      T val,
      typename enable_if<std::is_same<uint32_t, T> >::type* = 0) {
    return JSVal(val, detail::UInt32Tag());
  }

  template<typename T>
  static inline JSVal UInt16(
      T val,
      typename enable_if<std::is_same<uint16_t, T> >::type* = 0) {
    return JSVal(val, detail::UInt16Tag());
  }

  template<typename T>
  static inline JSVal Int32(
      T val,
      typename enable_if<std::is_same<int32_t, T> >::type* = 0) {
    return JSVal(val, detail::Int32Tag());
  }

  template<typename T>
  static inline JSVal Ptr(T* ptr) {
    return JSVal(ptr, detail::OtherPtrTag());
  }

 protected:
  JSVal(uint32_t val, detail::UInt32Tag dummy)
    : value_() {
    set_value_uint32(val);
  }

  JSVal(uint16_t val, detail::UInt16Tag dummy)
    : value_() {
    set_value_int32(val);
  }

  JSVal(int32_t val, detail::Int32Tag dummy)
    : value_() {
    set_value_int32(val);
  }

  template<typename T>
  JSVal(T* val, detail::OtherPtrTag dummy)
    : value_() {
    set_value_ptr(val);
  }

  value_type value_;
};

inline bool JSTrue(JSVal x,
                   detail::JSTrueType dummy = detail::JSTrueType()) {
  return x.IsBoolean() && x.boolean();
}

inline bool JSFalse(JSVal x,
                    detail::JSFalseType dummy = detail::JSFalseType()) {
  return x.IsBoolean() && !x.boolean();
}

inline bool JSNull(JSVal x,
                   detail::JSNullType dummy = detail::JSNullType()) {
  return x.IsNull();
}

inline bool JSUndefined(
    JSVal x, detail::JSUndefinedType dummy = detail::JSUndefinedType()) {
  return x.IsUndefined();
}

inline bool JSEmpty(JSVal x,
                    detail::JSEmptyType dummy = detail::JSEmptyType()) {
  return x.IsEmpty();
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
    if (core::IsNaN(lhsv) && core::IsNaN(rhsv)) {
      return true;
    }
    if (lhsv == rhsv) {
      if (core::Signbit(lhsv)) {
        if (core::Signbit(rhsv)) {
          return true;
        } else {
          return false;
        }
      } else {
        if (core::Signbit(rhsv)) {
          return false;
        } else {
          return true;
        }
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
#endif  // IV_LV5_JSVAL_H_
