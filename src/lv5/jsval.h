#ifndef IV_LV5_JSVAL_H_
#define IV_LV5_JSVAL_H_
#include "detail/array.h"
#include "dtoa.h"
#include "bit_cast.h"
#include "lv5/error_check.h"
#include "lv5/jsval_fwd.h"
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"
#include "lv5/jsenv.h"
#include "lv5/jsreference.h"
#include "lv5/jsbooleanobject.h"
#include "lv5/jsnumberobject.h"
#include "lv5/jsstringobject.h"
namespace iv {
namespace lv5 {

#if defined(IV_64) && !defined(IV_OS_SOLARIS)
// 64bit version
//
// Pointer value is not use higher 16bits so format is like this,
//   0000 XXXX XXXX XXXX
// we shift double values. so, if value is not pointer, top 16bit is not 0.
// and, right 3 bits of heap pointer value is always 0 (alignment), so set tag
// bit to this and use this space for other values (not number)
//
// pointer favor NaN boxing implemented in JSC,
// and double favor NaN boxing is implemented in LuaJIT and SpiderMonkey.
// we currently use BoehmGC, cannot use dirty pointer bit,
// choose pointer favor NaN boxing.
namespace detail {
namespace jsval64 {

static const uint64_t kOtherValueTag = UINT64_C(0x0000000000000002);
static const uint64_t kBooleanTag    = UINT64_C(0x0000000000000004);
static const uint64_t kUndefinedTag  = UINT64_C(0x0000000000000008);
static const uint64_t kNumberMask    = UINT64_C(0xFFFF000000000000);
static const uint64_t kDoubleOffset  = UINT64_C(0x0001000000000000);

// value mask (not cell)
static const uint64_t kValueMask = kNumberMask | kOtherValueTag;

// values
//  false:      0110
//  true:       0111
//  undefined:  1010
//  null:       0010
//  empty:      0000
static const uint64_t kFalse = kOtherValueTag | kBooleanTag | 0x0;
static const uint64_t kTrue = kOtherValueTag | kBooleanTag | 0x1;
static const uint64_t kUndefined = kOtherValueTag | kUndefinedTag;
static const uint64_t kNull = kOtherValueTag;
static const uint64_t kEmpty = 0x0;
static const uint64_t kNaN = core::BitCast<uint64_t>(core::kNaN) + kDoubleOffset;

} }  // namespace detail::jsval64

bool JSVal::IsEmpty() const {
  return value_.bytes_ == detail::jsval64::kEmpty;
}

bool JSVal::IsUndefined() const {
  return value_.bytes_ == detail::jsval64::kUndefined;
}

bool JSVal::IsNull() const {
  return value_.bytes_ == detail::jsval64::kNull;
}

bool JSVal::IsBoolean() const {
  // rm LSB (true or false) and check other bit pattern is Boolean
  return (value_.bytes_ & ~1) == detail::jsval64::kFalse;
}

bool JSVal::IsInt32() const {
  return (value_.bytes_ & detail::jsval64::kNumberMask)
      == detail::jsval64::kNumberMask;
}

bool JSVal::IsNumber() const {
  return value_.bytes_ & detail::jsval64::kNumberMask;
}

bool JSVal::IsCell() const {
  return !(value_.bytes_ & detail::jsval64::kValueMask);
}

radio::Cell* JSVal::cell() const {
  assert(IsCell());
  return static_cast<radio::Cell*>(value_.cell_);
}

bool JSVal::IsString() const {
  return IsCell() && cell()->tag() == radio::STRING;
}

bool JSVal::IsObject() const {
  return IsCell() && cell()->tag() == radio::OBJECT;
}

bool JSVal::IsReference() const {
  return IsCell() && cell()->tag() == radio::REFERENCE;
}

bool JSVal::IsEnvironment() const {
  return IsCell() && cell()->tag() == radio::ENVIRONMENT;
}

bool JSVal::IsOtherCell() const {
  return IsCell() && cell()->tag() == radio::POINTER;
}

bool JSVal::IsPrimitive() const {
  return IsNumber() || IsString() || IsBoolean();
}

JSReference* JSVal::reference() const {
  assert(IsReference());
  return static_cast<JSReference*>(value_.cell_);
}

JSEnv* JSVal::environment() const {
  assert(IsEnvironment());
  return static_cast<JSEnv*>(value_.cell_);
}

JSString* JSVal::string() const {
  assert(IsString());
  return static_cast<JSString*>(value_.cell_);
}

JSObject* JSVal::object() const {
  assert(IsObject());
  return static_cast<JSObject*>(value_.cell_);
}

bool JSVal::boolean() const {
  assert(IsBoolean());
  return value_.bytes_ & 0x1;
}

int32_t JSVal::int32() const {
  assert(IsInt32());
  return static_cast<int32_t>(value_.bytes_);
}

double JSVal::number() const {
  assert(IsNumber());
  if (IsInt32()) {
    return int32();
  } else {
    return core::BitCast<double>(value_.bytes_ - detail::jsval64::kDoubleOffset);
  }
}

void JSVal::set_value_int32(int32_t val) {
  // NUMBERMASK | 32bit pattern of int32_t
  value_.bytes_ = detail::jsval64::kNumberMask | static_cast<uint32_t>(val);
}

void JSVal::set_value_uint32(uint32_t val) {
  if (static_cast<int32_t>(val) < 0) {  // LSB is 1
    value_.bytes_ =
        core::BitCast<uint64_t>(static_cast<double>(val)) + detail::jsval64::kDoubleOffset;
  } else {
    set_value_int32(static_cast<int32_t>(val));
  }
}

void JSVal::set_value(double val) {
  const int32_t i = static_cast<int32_t>(val);
  if (val != i || (!i && core::Signbit(val))) {
    // this value is not represented by int32_t
    value_.bytes_ = (val == val) ?
        (core::BitCast<uint64_t>(val) + detail::jsval64::kDoubleOffset) :
        detail::jsval64::kNaN;
  } else {
    set_value_int32(i);
  }
}

void JSVal::set_value_cell(radio::Cell* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSVal::set_value(JSObject* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSVal::set_value(JSString* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSVal::set_value(JSReference* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSVal::set_value(JSEnv* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSVal::set_value(JSTrueKeywordType val) {
  value_.bytes_ = detail::jsval64::kTrue;
}

void JSVal::set_value(JSFalseKeywordType val) {
  value_.bytes_ = detail::jsval64::kFalse;
}

void JSVal::set_value(JSNaNKeywordType val) {
  value_.bytes_ = detail::jsval64::kNaN;
}

void JSVal::set_null() {
  value_.bytes_ = detail::jsval64::kNull;
}

void JSVal::set_undefined() {
  value_.bytes_ = detail::jsval64::kUndefined;
}

void JSVal::set_empty() {
  value_.bytes_ = detail::jsval64::kEmpty;
}

bool JSVal::SameValue(const this_type& lhs, const this_type& rhs) {
  if (lhs.IsInt32() && rhs.IsInt32()) {
    return lhs.int32() == rhs.int32();
  }

  if (lhs.IsNumber() && rhs.IsNumber()) {
    const double lhsv = lhs.number();
    const double rhsv = rhs.number();
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

  // if not cell, check bit pattern
  if (!lhs.IsCell() || !rhs.IsCell()) {
    return lhs.value_.bytes_ == rhs.value_.bytes_;
  }

  if (lhs.IsString() && rhs.IsString()) {
    return *(lhs.string()) == *(rhs.string());
  }

  return lhs.value_.bytes_ == rhs.value_.bytes_;
}

bool JSVal::StrictEqual(const this_type& lhs, const this_type& rhs) {
  if (lhs.IsInt32() && rhs.IsInt32()) {
    return lhs.int32() == rhs.int32();
  }

  if (lhs.IsNumber() && rhs.IsNumber()) {
    return lhs.number() == rhs.number();
  }

  // if not cell, check bit pattern
  if (!lhs.IsCell() || !rhs.IsCell()) {
    return lhs.value_.bytes_ == rhs.value_.bytes_;
  }

  if (lhs.IsString() && rhs.IsString()) {
    return *(lhs.string()) == *(rhs.string());
  }

  return lhs.value_.bytes_ == rhs.value_.bytes_;
}

#else
// 32bit version (or 128bit JSVal in Solaris)
//
// NaN boxing 32bit
// according to IEEE754, if signbit and exponent bit (1 + 11 = 12bit) are 1s,
// value is NaN. so we use top 16bit as tag and canonicalize NaN only to one
// kind bit pattern, use other NaN bit pattern as ptr, int32_t, etc...
namespace detail {
namespace jsval32 {

static const uint32_t kOtherCellTag   = 0xffffffff;  // cell range end
static const uint32_t kEnvironmentTag = 0xfffffffe;
static const uint32_t kReferenceTag   = 0xfffffffd;
static const uint32_t kStringTag      = 0xfffffffc;
static const uint32_t kObjectTag      = 0xfffffffb;  // cell range start
static const uint32_t kEmptyTag       = 0xfffffffa;
static const uint32_t kUndefinedTag   = 0xfffffff9;
static const uint32_t kNullTag        = 0xfffffff8;
static const uint32_t kBoolTag        = 0xfffffff7;
static const uint32_t kNumberTag      = 0xfffffff5;
static const uint32_t kInt32Tag       = 0xfffffff4;

inline bool InPtrRange(uint32_t tag) {
  return kObjectTag <= tag;
}

inline uint32_t GetType(const JSVal& val) {
  return val.IsNumber() ? kNumberTag : val.Layout().struct_.tag_;
}

} }  // namespace detail::jsval32

bool JSVal::IsEmpty() const {
  return value_.struct_.tag_ == detail::jsval32::kEmptyTag;
}

bool JSVal::IsUndefined() const {
  return value_.struct_.tag_ == detail::jsval32::kUndefinedTag;
}

bool JSVal::IsNull() const {
  return value_.struct_.tag_ == detail::jsval32::kNullTag;
}

bool JSVal::IsBoolean() const {
  return value_.struct_.tag_ == detail::jsval32::kBoolTag;
}

bool JSVal::IsString() const {
  return value_.struct_.tag_ == detail::jsval32::kStringTag;
}

bool JSVal::IsObject() const {
  return value_.struct_.tag_ == detail::jsval32::kObjectTag;
}

bool JSVal::IsInt32() const {
  return value_.struct_.tag_ == detail::jsval32::kInt32Tag;
}

bool JSVal::IsNumber() const {
  return value_.struct_.tag_ < detail::jsval32::kNumberTag;
}

bool JSVal::IsReference() const {
  return value_.struct_.tag_ == detail::jsval32::kReferenceTag;
}

bool JSVal::IsEnvironment() const {
  return value_.struct_.tag_ == detail::jsval32::kEnvironmentTag;
}

bool JSVal::IsOtherCell() const {
  return value_.struct_.tag_ == detail::jsval32::kOtherCellTag;
}

bool JSVal::IsCell() const {
  return detail::jsval32::InPtrRange(value_.struct_.tag_);
}

bool JSVal::IsPrimitive() const {
  return IsNumber() || IsString() || IsBoolean();
}

JSReference* JSVal::reference() const {
  assert(IsReference());
  return value_.struct_.payload_.reference_;
}

JSEnv* JSVal::environment() const {
  assert(IsEnvironment());
  return value_.struct_.payload_.environment_;
}

JSString* JSVal::string() const {
  assert(IsString());
  return value_.struct_.payload_.string_;
}

JSObject* JSVal::object() const {
  assert(IsObject());
  return value_.struct_.payload_.object_;
}

radio::Cell* JSVal::cell() const {
  assert(IsCell());
  return value_.struct_.payload_.cell_;
}

bool JSVal::boolean() const {
  assert(IsBoolean());
  return value_.struct_.payload_.boolean_;
}

int32_t JSVal::int32() const {
  assert(IsInt32());
  return value_.struct_.payload_.int32_;
}

double JSVal::number() const {
  assert(IsNumber());
  if (IsInt32()) {
    return int32();
  } else {
    return value_.number_.as_;
  }
}

void JSVal::set_value_cell(radio::Cell* ptr) {
  value_.struct_.payload_.cell_ = ptr;
  value_.struct_.tag_ = detail::jsval32::kOtherCellTag;
}

void JSVal::set_value_int32(int32_t val) {
  value_.struct_.payload_.int32_ = val;
  value_.struct_.tag_ = detail::jsval32::kInt32Tag;
}

void JSVal::set_value_uint32(uint32_t val) {
  if (static_cast<int32_t>(val) < 0) {  // LSB is 1
    value_.number_.as_ = val;
  } else {
    set_value_int32(static_cast<int32_t>(val));
  }
}

void JSVal::set_value(double val) {
  const int32_t i = static_cast<int32_t>(val);
  if (val != i || (!i && core::Signbit(val))) {
    // this value is not represented by int32_t
    value_.number_.as_ = (val == val) ? val : core::kNaN;
  } else {
    set_value_int32(i);
  }
}

void JSVal::set_value(JSObject* val) {
  value_.struct_.payload_.object_ = val;
  value_.struct_.tag_ = detail::jsval32::kObjectTag;
}

void JSVal::set_value(JSString* val) {
  value_.struct_.payload_.string_ = val;
  value_.struct_.tag_ = detail::jsval32::kStringTag;
}

void JSVal::set_value(JSReference* ref) {
  value_.struct_.payload_.reference_ = ref;
  value_.struct_.tag_ = detail::jsval32::kReferenceTag;
}

void JSVal::set_value(JSEnv* ref) {
  value_.struct_.payload_.environment_ = ref;
  value_.struct_.tag_ = detail::jsval32::kEnvironmentTag;
}

void JSVal::set_value(JSTrueKeywordType val) {
  value_.struct_.payload_.boolean_ = true;
  value_.struct_.tag_ = detail::jsval32::kBoolTag;
}

void JSVal::set_value(JSFalseKeywordType val) {
  value_.struct_.payload_.boolean_ = false;
  value_.struct_.tag_ = detail::jsval32::kBoolTag;
}

void JSVal::set_value(JSNaNKeywordType val) {
  value_.number_.as_ = core::kNaN;
}

void JSVal::set_null() {
  value_.struct_.tag_ = detail::jsval32::kNullTag;
}

void JSVal::set_undefined() {
  value_.struct_.tag_ = detail::jsval32::kUndefinedTag;
}

void JSVal::set_empty() {
  value_.struct_.tag_ = detail::jsval32::kEmptyTag;
}

bool JSVal::SameValue(const this_type& lhs, const this_type& rhs) {
  if (detail::jsval32::GetType(lhs) != detail::jsval32::GetType(rhs)) {
    return false;
  }
  if (lhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber()) {
    const double lhsv = lhs.number();
    const double rhsv = rhs.number();
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

bool JSVal::StrictEqual(const this_type& lhs, const this_type& rhs) {
  if (detail::jsval32::GetType(lhs) != detail::jsval32::GetType(rhs)) {
    return false;
  }
  if (lhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber()) {
    // if some value is NaN, always returns false
    return lhs.number() == rhs.number();
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
#endif

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

JSObject* JSVal::GetPrimitiveProto(Context* ctx) const {
  assert(IsPrimitive());
  if (IsString()) {
    return context::GetClassSlot(ctx, Class::String).prototype;
  } else if (IsNumber()) {
    return context::GetClassSlot(ctx, Class::Number).prototype;
  } else {  // IsBoolean()
    return context::GetClassSlot(ctx, Class::Boolean).prototype;
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
    const char* const str =
        core::DoubleToCString(number(), buffer.data(), buffer.size());
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
    return core::StringToDouble(*string()->GetFiber(), false);
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

bool JSVal::ToBoolean(Error* e) const {
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

JSVal JSVal::ToPrimitive(Context* ctx, Hint::Object hint, Error* e) const {
  if (IsObject()) {
    return object()->DefaultValue(ctx, hint, e);
  } else {
    assert(!IsEnvironment() && !IsReference() && !IsEmpty());
    return *this;
  }
}

int32_t JSVal::ToInt32(Context* ctx, Error* e) const {
  if (IsInt32()) {
    return int32();
  } else {
    return core::DoubleToInt32(ToNumber(ctx, e));
  }
}

uint32_t JSVal::ToUInt32(Context* ctx, Error* e) const {
  if (IsInt32() && int32() >= 0) {
    return static_cast<uint32_t>(int32());
  } else {
    return core::DoubleToUInt32(ToNumber(ctx, e));
  }
}

uint32_t JSVal::GetUInt32() const {
  assert(IsNumber());
  uint32_t val = 0;  // make gcc happy
  GetUInt32(&val);
  return val;
}

bool JSVal::GetUInt32(uint32_t* result) const {
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
#endif  // IV_LV5_JSVAL_H_
