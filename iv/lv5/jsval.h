#ifndef IV_LV5_JSVAL_H_
#define IV_LV5_JSVAL_H_
#include <iv/detail/array.h>
#include <iv/dtoa.h>
#include <iv/bit_cast.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/context.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsreference.h>
#include <iv/lv5/jsbooleanobject.h>
#include <iv/lv5/jsnumberobject.h>
#include <iv/lv5/jsstringobject.h>
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
static const uint64_t kNaN =
    core::BitCast<uint64_t>(core::kNaN) + kDoubleOffset;

} }  // namespace detail::jsval64

bool JSLayout::IsEmpty() const {
  return value_.bytes_ == detail::jsval64::kEmpty;
}

bool JSLayout::IsUndefined() const {
  return value_.bytes_ == detail::jsval64::kUndefined;
}

bool JSLayout::IsNull() const {
  return value_.bytes_ == detail::jsval64::kNull;
}

bool JSLayout::IsNullOrUndefined() const {
  // (1000)2 = 8
  // Null is (0010)2 and Undefined is (1010)2
  return (value_.bytes_ & ~8) == detail::jsval64::kNull;
}

bool JSLayout::IsBoolean() const {
  // rm LSB (true or false) and check other bit pattern is Boolean
  return (value_.bytes_ & ~1) == detail::jsval64::kFalse;
}

bool JSLayout::IsInt32() const {
  return (value_.bytes_ & detail::jsval64::kNumberMask)
      == detail::jsval64::kNumberMask;
}

bool JSLayout::IsNumber() const {
  return value_.bytes_ & detail::jsval64::kNumberMask;
}

bool JSLayout::IsCell() const {
  return !(value_.bytes_ & detail::jsval64::kValueMask) && !IsEmpty();
}

radio::Cell* JSLayout::cell() const {
  assert(IsCell());
  return value_.cell_;
}

bool JSLayout::IsString() const {
  return IsCell() && cell()->tag() == radio::STRING;
}

bool JSLayout::IsObject() const {
  return IsCell() && cell()->tag() == radio::OBJECT;
}

bool JSLayout::IsReference() const {
  return IsCell() && cell()->tag() == radio::REFERENCE;
}

bool JSLayout::IsEnvironment() const {
  return IsCell() && cell()->tag() == radio::ENVIRONMENT;
}

bool JSLayout::IsOtherCell() const {
  return IsCell() && cell()->tag() == radio::POINTER;
}

bool JSLayout::IsPrimitive() const {
  return IsNumber() || IsString() || IsBoolean();
}

JSReference* JSLayout::reference() const {
  assert(IsReference());
  return static_cast<JSReference*>(value_.cell_);
}

JSEnv* JSLayout::environment() const {
  assert(IsEnvironment());
  return static_cast<JSEnv*>(value_.cell_);
}

JSString* JSLayout::string() const {
  assert(IsString());
  return static_cast<JSString*>(value_.cell_);
}

JSObject* JSLayout::object() const {
  assert(IsObject());
  return static_cast<JSObject*>(value_.cell_);
}

bool JSLayout::boolean() const {
  assert(IsBoolean());
  return value_.bytes_ & 0x1;
}

int32_t JSLayout::int32() const {
  assert(IsInt32());
  return static_cast<int32_t>(value_.bytes_);
}

double JSLayout::number() const {
  assert(IsNumber());
  if (IsInt32()) {
    return int32();
  } else {
    return core::BitCast<double>(
        value_.bytes_ - detail::jsval64::kDoubleOffset);
  }
}

void JSLayout::set_value_int32(int32_t val) {
  // NUMBERMASK | 32bit pattern of int32_t
  value_.bytes_ = detail::jsval64::kNumberMask | static_cast<uint32_t>(val);
}

void JSLayout::set_value_uint32(uint32_t val) {
  if (static_cast<int32_t>(val) < 0) {  // LSB is 1
    value_.bytes_ =
        core::BitCast<uint64_t>(
            static_cast<double>(val)) + detail::jsval64::kDoubleOffset;
  } else {
    set_value_int32(static_cast<int32_t>(val));
  }
}

void JSLayout::set_value(double val) {
  const int32_t i = static_cast<int32_t>(val);
  if (val != i || (!i && core::math::Signbit(val))) {
    // this value is not represented by int32_t
    value_.bytes_ = (val == val) ?
        (core::BitCast<uint64_t>(val) + detail::jsval64::kDoubleOffset) :
        detail::jsval64::kNaN;
  } else {
    set_value_int32(i);
  }
}

void JSLayout::set_value_cell(radio::Cell* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSLayout::set_value(JSObject* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSLayout::set_value(JSString* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSLayout::set_value(JSReference* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSLayout::set_value(JSEnv* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

void JSLayout::set_value(detail::JSTrueType val) {
  value_.bytes_ = detail::jsval64::kTrue;
}

void JSLayout::set_value(detail::JSFalseType val) {
  value_.bytes_ = detail::jsval64::kFalse;
}

void JSLayout::set_value(detail::JSNaNType val) {
  value_.bytes_ = detail::jsval64::kNaN;
}

void JSLayout::set_null() {
  value_.bytes_ = detail::jsval64::kNull;
}

void JSLayout::set_undefined() {
  value_.bytes_ = detail::jsval64::kUndefined;
}

void JSLayout::set_empty() {
  value_.bytes_ = detail::jsval64::kEmpty;
}

bool JSLayout::SameValue(this_type lhs, this_type rhs) {
  if (lhs.IsInt32()) {
    if (rhs.IsInt32()) {
      return lhs.int32() == rhs.int32();
    }
    // because +0(int32_t) and -0(double) is not the same value
    return false;
  } else if (lhs.IsNumber()) {
    if (rhs.IsInt32() || !rhs.IsNumber()) {
      return false;
    }
    const double lhsv = lhs.number();
    const double rhsv = rhs.number();
    if (lhsv == rhsv) {
      // when control flow comes to here, lhs and rhs is -0 and -0
      return true;
    }
    return core::math::IsNaN(lhsv) && core::math::IsNaN(rhsv);
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

bool JSLayout::StrictEqual(this_type lhs, this_type rhs) {
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

inline std::size_t JSVal::Hasher::operator()(JSVal val) const {
  if (val.IsInt32()) {
    return std::hash<int32_t>()(val.int32());
  }

  if (val.IsNumber()) {
    const double d = val.number();
    if (core::math::IsNaN(d)) {
      return std::hash<double>()(core::kNaN);
    }
    return std::hash<double>()(d);
  }

  if (val.IsString()) {
    JSString* str = val.string();
    if (str->Is8Bit()) {
      return core::Hash::StringToHash(*str->Get8Bit());
    } else {
      return core::Hash::StringToHash(*str->Get16Bit());
    }
  }

  return std::hash<uint64_t>()(val.value_.bytes_);
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

inline uint32_t GetType(const JSLayout& val) {
  return val.IsNumber() ? kNumberTag : val.Layout().struct_.tag_;
}

} }  // namespace detail::jsval32

bool JSLayout::IsEmpty() const {
  return value_.struct_.tag_ == detail::jsval32::kEmptyTag;
}

bool JSLayout::IsUndefined() const {
  return value_.struct_.tag_ == detail::jsval32::kUndefinedTag;
}

bool JSLayout::IsNull() const {
  return value_.struct_.tag_ == detail::jsval32::kNullTag;
}

bool JSLayout::IsNullOrUndefined() const {
  return IsNull() || IsUndefined();
}

bool JSLayout::IsBoolean() const {
  return value_.struct_.tag_ == detail::jsval32::kBoolTag;
}

bool JSLayout::IsString() const {
  return value_.struct_.tag_ == detail::jsval32::kStringTag;
}

bool JSLayout::IsObject() const {
  return value_.struct_.tag_ == detail::jsval32::kObjectTag;
}

bool JSLayout::IsInt32() const {
  return value_.struct_.tag_ == detail::jsval32::kInt32Tag;
}

bool JSLayout::IsNumber() const {
  return value_.struct_.tag_ < detail::jsval32::kNumberTag;
}

bool JSLayout::IsReference() const {
  return value_.struct_.tag_ == detail::jsval32::kReferenceTag;
}

bool JSLayout::IsEnvironment() const {
  return value_.struct_.tag_ == detail::jsval32::kEnvironmentTag;
}

bool JSLayout::IsOtherCell() const {
  return value_.struct_.tag_ == detail::jsval32::kOtherCellTag;
}

bool JSLayout::IsCell() const {
  return detail::jsval32::InPtrRange(value_.struct_.tag_);
}

bool JSLayout::IsPrimitive() const {
  return IsNumber() || IsString() || IsBoolean();
}

JSReference* JSLayout::reference() const {
  assert(IsReference());
  return value_.struct_.payload_.reference_;
}

JSEnv* JSLayout::environment() const {
  assert(IsEnvironment());
  return value_.struct_.payload_.environment_;
}

JSString* JSLayout::string() const {
  assert(IsString());
  return value_.struct_.payload_.string_;
}

JSObject* JSLayout::object() const {
  assert(IsObject());
  return value_.struct_.payload_.object_;
}

radio::Cell* JSLayout::cell() const {
  assert(IsCell());
  return value_.struct_.payload_.cell_;
}

bool JSLayout::boolean() const {
  assert(IsBoolean());
  return value_.struct_.payload_.boolean_;
}

int32_t JSLayout::int32() const {
  assert(IsInt32());
  return value_.struct_.payload_.int32_;
}

double JSLayout::number() const {
  assert(IsNumber());
  if (IsInt32()) {
    return int32();
  } else {
    return value_.number_.as_;
  }
}

void JSLayout::set_value_cell(radio::Cell* ptr) {
  value_.struct_.payload_.cell_ = ptr;
  value_.struct_.tag_ = detail::jsval32::kOtherCellTag;
}

void JSLayout::set_value_int32(int32_t val) {
  value_.struct_.payload_.int32_ = val;
  value_.struct_.tag_ = detail::jsval32::kInt32Tag;
}

void JSLayout::set_value_uint32(uint32_t val) {
  if (static_cast<int32_t>(val) < 0) {  // LSB is 1
    value_.number_.as_ = val;
  } else {
    set_value_int32(static_cast<int32_t>(val));
  }
}

void JSLayout::set_value(double val) {
  const int32_t i = static_cast<int32_t>(val);
  if (val != i || (!i && core::math::Signbit(val))) {
    // this value is not represented by int32_t
    value_.number_.as_ = (val == val) ? val : core::kNaN;
  } else {
    set_value_int32(i);
  }
}

void JSLayout::set_value(JSObject* val) {
  value_.struct_.payload_.object_ = val;
  value_.struct_.tag_ = detail::jsval32::kObjectTag;
}

void JSLayout::set_value(JSString* val) {
  value_.struct_.payload_.string_ = val;
  value_.struct_.tag_ = detail::jsval32::kStringTag;
}

void JSLayout::set_value(JSReference* ref) {
  value_.struct_.payload_.reference_ = ref;
  value_.struct_.tag_ = detail::jsval32::kReferenceTag;
}

void JSLayout::set_value(JSEnv* ref) {
  value_.struct_.payload_.environment_ = ref;
  value_.struct_.tag_ = detail::jsval32::kEnvironmentTag;
}

void JSLayout::set_value(detail::JSTrueType val) {
  value_.struct_.payload_.boolean_ = true;
  value_.struct_.tag_ = detail::jsval32::kBoolTag;
}

void JSLayout::set_value(detail::JSFalseType val) {
  value_.struct_.payload_.boolean_ = false;
  value_.struct_.tag_ = detail::jsval32::kBoolTag;
}

void JSLayout::set_value(detail::JSNaNType val) {
  value_.number_.as_ = core::kNaN;
}

void JSLayout::set_null() {
  value_.struct_.tag_ = detail::jsval32::kNullTag;
}

void JSLayout::set_undefined() {
  value_.struct_.tag_ = detail::jsval32::kUndefinedTag;
}

void JSLayout::set_empty() {
  value_.struct_.tag_ = detail::jsval32::kEmptyTag;
}

bool JSLayout::SameValue(this_type lhs, this_type rhs) {
  if (detail::jsval32::GetType(lhs) != detail::jsval32::GetType(rhs)) {
    return false;
  }
  if (lhs.IsNullOrUndefined()) {
    return true;
  }
  if (lhs.IsNumber()) {
    const double lhsv = lhs.number();
    const double rhsv = rhs.number();
    if (lhsv == rhsv) {
      return core::math::Signbit(lhsv) == core::math::Signbit(rhsv);
    } else {
      return core::math::IsNaN(lhsv) && core::math::IsNaN(rhsv);
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

bool JSLayout::StrictEqual(this_type lhs, this_type rhs) {
  if (detail::jsval32::GetType(lhs) != detail::jsval32::GetType(rhs)) {
    return false;
  }
  if (lhs.IsNullOrUndefined()) {
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

inline std::size_t JSVal::Hasher::operator()(JSVal val) const {
  if (val.IsInt32()) {
    return std::hash<int32_t>()(val.int32());
  }

  if (val.IsNumber()) {
    const double d = val.number();
    if (core::math::IsNaN(d)) {
      return std::hash<double>()(core::kNaN);
    }
    return std::hash<double>()(d);
  }

  if (val.IsString()) {
    JSString* str = val.string();
    if (str->Is8Bit()) {
      return core::Hash::StringToHash(*str->Get8Bit());
    } else {
      return core::Hash::StringToHash(*str->Get16Bit());
    }
  }

  if (val.IsCell()) {
    return std::hash<radio::Cell*>()(val.cell());
  }

  if (val.IsBoolean()) {
    return std::hash<uint32_t>()(val.boolean());
  }

  return std::hash<uint32_t>()(detail::jsval32::GetType(val));
}

#endif  // 32 or 64 bit JSLayout ifdef

JSString* JSLayout::TypeOf(Context* ctx) const {
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
    return ctx->global_data()->string_undefined();
  }
}

JSObject* JSLayout::GetPrimitiveProto(Context* ctx) const {
  assert(IsPrimitive());
  if (IsString()) {
    return context::GetClassSlot(ctx, Class::String).prototype;
  } else if (IsNumber()) {
    return context::GetClassSlot(ctx, Class::Number).prototype;
  } else {  // IsBoolean()
    return context::GetClassSlot(ctx, Class::Boolean).prototype;
  }
}

JSObject* JSLayout::ToObject(Context* ctx, Error* e) const {
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

JSString* JSLayout::ToString(Context* ctx, Error* e) const {
  if (IsString()) {
    return string();
  } else if (IsInt32()) {
    // int32 short cut
    std::array<char, 15> buffer;
    char* end = core::Int32ToString(int32(), buffer.data());
    return JSString::New(ctx, buffer.data(), end, true);
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
    return ctx->global_data()->string_undefined();
  } else {
    assert(IsObject());
    JSLayout prim = object()->DefaultValue(ctx, Hint::STRING,
                                        IV_LV5_ERROR_WITH(e, NULL));
    return prim.ToString(ctx, e);
  }
}

Symbol JSLayout::ToSymbol(Context* ctx, Error* e) const {
  uint32_t index;
  if (GetUInt32(&index)) {
    return symbol::MakeSymbolFromIndex(index);
  } else {
    if (IsString()) {
      return context::Intern(ctx, string());
    } else if (IsNumber()) {
      // int32 short cut
      if (IsInt32()) {
        std::array<char, 15> buffer;
        char* end = core::Int32ToString(int32(), buffer.data());
        return context::Intern(
            ctx,
            core::StringPiece(buffer.data(),
                              std::distance(buffer.data(), end)));
      } else {
        std::array<char, 80> buffer;
        const char* const str =
            core::DoubleToCString(number(), buffer.data(), buffer.size());
        return context::Intern(ctx, str);
      }
    } else if (IsBoolean()) {
      return context::Intern(ctx, (boolean() ? "true" : "false"));
    } else if (IsNull()) {
      return context::Intern(ctx, "null");
    } else if (IsUndefined()) {
      return context::Intern(ctx, "undefined");
    } else {
      assert(IsObject());
      const JSLayout prim =
          object()->DefaultValue(
              ctx, Hint::STRING,
              IV_LV5_ERROR_WITH(e, symbol::kDummySymbol));
      return prim.ToSymbol(ctx, e);
    }
  }
}

core::UString JSLayout::ToUString(Context* ctx, Error* e) const {
  if (IsString()) {
    JSString* str = string();
    return core::UString(str->begin(), str->end());
  } else if (IsNumber()) {
    // int32 short cut
    if (IsInt32()) {
      std::array<char, 15> buffer;
      char* end = core::Int32ToString(int32(), buffer.data());
      return core::UString(buffer.data(), end);
    } else {
      std::array<char, 80> buffer;
      const char* const str =
          core::DoubleToCString(number(), buffer.data(), buffer.size());
      return core::UString(str, str + std::strlen(str));
    }
  } else if (IsBoolean()) {
    const char* str = (boolean()) ? "true" : "false";
    return core::UString(str, str + std::strlen(str));
  } else if (IsNull()) {
    const char* str = "null";
    return core::UString(str, str + std::strlen(str));
  } else if (IsUndefined()) {
    const char* str = "undefined";
    return core::UString(str, str + std::strlen(str));
  } else {
    assert(IsObject());
    const JSLayout prim =
        object()->DefaultValue(
            ctx, Hint::STRING,
            IV_LV5_ERROR_WITH(e, core::UString()));
    return prim.ToUString(ctx, e);
  }
}

double JSLayout::ToNumber(Context* ctx, Error* e) const {
  if (IsNumber()) {
    return number();
  } else if (IsString()) {
    const JSString* str = string();
    if (str->Is8Bit()) {
      return core::StringToDouble(*str->Get8Bit(), false);
    } else {
      return core::StringToDouble(*str->Get16Bit(), false);
    }
  } else if (IsBoolean()) {
    return boolean() ? 1 : +0;
  } else if (IsNull()) {
    return +0;
  } else if (IsUndefined()) {
    return core::kNaN;
  } else {
    assert(IsObject());
    JSLayout prim = object()->DefaultValue(ctx, Hint::NUMBER,
                                        IV_LV5_ERROR_WITH(e, 0.0));
    return prim.ToNumber(ctx, e);
  }
}

JSVal JSVal::ToNumberValue(Context* ctx, Error* e) const {
  if (IsNumber()) {
    return *this;
  } else if (IsString()) {
    const JSString* str = string();
    if (str->Is8Bit()) {
      return core::StringToDouble(*str->Get8Bit(), false);
    } else {
      return core::StringToDouble(*str->Get16Bit(), false);
    }
  } else if (IsBoolean()) {
    return JSVal::Int32(boolean() ? 1 : 0);
  } else if (IsNull()) {
    return JSVal::Int32(0);
  } else if (IsUndefined()) {
    return core::kNaN;
  } else {
    assert(IsObject());
    JSVal prim = object()->DefaultValue(ctx, Hint::NUMBER,
                                        IV_LV5_ERROR_WITH(e, JSVal::Int32(0)));
    return prim.ToNumber(ctx, e);
  }
}

bool JSLayout::ToBoolean() const {
  if (IsNumber()) {
    const double num = number();
    return num != 0 && !core::math::IsNaN(num);
  } else if (IsString()) {
    return !string()->empty();
  } else if (IsNullOrUndefined()) {
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

int32_t JSLayout::ToInt32(Context* ctx, Error* e) const {
  if (IsInt32()) {
    return int32();
  } else {
    return core::DoubleToInt32(ToNumber(ctx, e));
  }
}

uint32_t JSLayout::ToUInt32(Context* ctx, Error* e) const {
  if (IsInt32() && int32() >= 0) {
    return static_cast<uint32_t>(int32());
  } else {
    return core::DoubleToUInt32(ToNumber(ctx, e));
  }
}

uint32_t JSLayout::GetUInt32() const {
  assert(IsNumber());
  uint32_t val = 0;  // make gcc happy
  GetUInt32(&val);
  return val;
}

bool JSLayout::GetUInt32(uint32_t* result) const {
  if (IsInt32() && int32() >= 0) {
    *result = static_cast<uint32_t>(int32());
    return true;
  } else if (IsNumber()) {
    const double val = number();
    if (val == (*result = static_cast<uint32_t>(val))) {
      return true;
    }
  }
  return false;
}

bool JSLayout::IsCallable() const {
  return IsObject() && object()->IsCallable();
}

void JSLayout::CheckObjectCoercible(Error* e) const {
  assert(!IsEnvironment() && !IsReference() && !IsEmpty());
  if (IsNull()) {
    e->Report(Error::Type, "null has no properties");
  } else if (IsUndefined()) {
    e->Report(Error::Type, "undefined has no properties");
  }
}

#define CHECK IV_LV5_ERROR_WITH(e, false)
bool JSVal::AbstractEqual(Context* ctx,
                          this_type lhs,
                          this_type rhs, Error* e) {
  do {
    // equality phase
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return lhs.int32() == rhs.int32();
    }
    if (lhs.IsNumber() && rhs.IsNumber()) {
      return lhs.number() == rhs.number();
    }
    if (lhs.IsNullOrUndefined() && rhs.IsNullOrUndefined()) {
      return true;
    }
    if (lhs.IsString() && rhs.IsString()) {
      return *(lhs.string()) == *(rhs.string());
    }
    if (lhs.IsBoolean() && rhs.IsBoolean()) {
      return lhs.boolean() == rhs.boolean();
    }
    if (lhs.IsObject() && rhs.IsObject()) {
      return lhs.object() == rhs.object();
    }

    // conversion phase
    if (lhs.IsNumber() && rhs.IsString()) {
      rhs = rhs.ToNumber(ctx, CHECK);
      continue;
    }
    if (lhs.IsString() && rhs.IsNumber()) {
      lhs = lhs.ToNumber(ctx, CHECK);
      continue;
    }
    if (lhs.IsBoolean()) {
      lhs = lhs.ToNumber(ctx, CHECK);
      continue;
    }
    if (rhs.IsBoolean()) {
      rhs = rhs.ToNumber(ctx, CHECK);
      continue;
    }
    if ((lhs.IsString() || lhs.IsNumber()) && rhs.IsObject()) {
      rhs = rhs.ToPrimitive(ctx, Hint::NONE, CHECK);
      continue;
    }
    if (lhs.IsObject() && (rhs.IsString() || rhs.IsNumber())) {
      lhs = lhs.ToPrimitive(ctx, Hint::NONE, CHECK);
      continue;
    }
    return false;
  } while (true);
  return false;  // makes compiler happy
}
#undef CHECK

// section 11.8.5
template<bool LeftFirst>
CompareResult JSVal::Compare(Context* ctx,
                             this_type lhs,
                             this_type rhs, Error* e) {
  if (lhs.IsNumber() && rhs.IsNumber()) {
    return NumberCompare(lhs.number(), rhs.number());
  }

  JSVal px;
  JSVal py;
  if (LeftFirst) {
    px = lhs.ToPrimitive(ctx, Hint::NUMBER,
                         IV_LV5_ERROR_WITH(e, CMP_UNDEFINED));
    py = rhs.ToPrimitive(ctx, Hint::NUMBER,
                         IV_LV5_ERROR_WITH(e, CMP_UNDEFINED));
  } else {
    py = rhs.ToPrimitive(ctx, Hint::NUMBER,
                         IV_LV5_ERROR_WITH(e, CMP_UNDEFINED));
    px = lhs.ToPrimitive(ctx, Hint::NUMBER,
                         IV_LV5_ERROR_WITH(e, CMP_UNDEFINED));
  }

  // fast case
  if (px.IsInt32() && py.IsInt32()) {
    return (px.int32() < py.int32()) ? CMP_TRUE : CMP_FALSE;
  } else if (px.IsString() && py.IsString()) {
    // step 4
    return (*(px.string()) < *(py.string())) ? CMP_TRUE : CMP_FALSE;
  } else {
    const double nx = px.ToNumber(ctx, IV_LV5_ERROR_WITH(e, CMP_UNDEFINED));
    return NumberCompare(nx, py.ToNumber(ctx, e));
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSVAL_H_
