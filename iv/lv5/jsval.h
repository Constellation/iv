#ifndef IV_LV5_JSVAL_H_
#define IV_LV5_JSVAL_H_
#include <iv/detail/array.h>
#include <iv/dtoa.h>
#include <iv/bit_cast.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsreference.h>
#include <iv/lv5/jsbooleanobject.h>
#include <iv/lv5/jsnumberobject.h>
#include <iv/lv5/jsstringobject.h>
#include <iv/lv5/jssymbolobject.h>
#include <iv/lv5/jssymbol.h>
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
static const uint64_t kBooleanRepresentation = kOtherValueTag | kBooleanTag;
static const uint64_t kFalse = kBooleanRepresentation | 0x0;
static const uint64_t kTrue = kBooleanRepresentation | 0x1;
static const uint64_t kUndefined = kOtherValueTag | kUndefinedTag;
static const uint64_t kNull = kOtherValueTag;
static const uint64_t kEmpty = 0x0;
static const uint64_t kNaN =
    core::BitCast<uint64_t>(core::kNaN) + kDoubleOffset;

} }  // namespace detail::jsval64

inline bool JSLayout::IsEmpty() const {
  return value_.bytes_ == detail::jsval64::kEmpty;
}

inline bool JSLayout::IsUndefined() const {
  return value_.bytes_ == detail::jsval64::kUndefined;
}

inline bool JSLayout::IsNull() const {
  return value_.bytes_ == detail::jsval64::kNull;
}

inline bool JSLayout::IsNullOrUndefined() const {
  // (1000)2 = 8
  // Null is (0010)2 and Undefined is (1010)2
  return (value_.bytes_ & ~8) == detail::jsval64::kNull;
}

inline bool JSLayout::IsBoolean() const {
  // rm LSB (true or false) and check other bit pattern is Boolean
  return (value_.bytes_ & ~1) == detail::jsval64::kFalse;
}

inline bool JSLayout::IsInt32() const {
  return value_.bytes_ >= detail::jsval64::kNumberMask;
}

inline bool JSLayout::IsNumber() const {
  return value_.bytes_ & detail::jsval64::kNumberMask;
}

inline bool JSLayout::IsCell() const {
  return !(value_.bytes_ & detail::jsval64::kValueMask) && !IsEmpty();
}

inline radio::Cell* JSLayout::cell() const {
  assert(IsCell());
  return value_.cell_;
}

inline bool JSLayout::IsString() const {
  return IsCell() && cell()->tag() == radio::STRING;
}

inline bool JSLayout::IsSymbol() const {
  return IsCell() && cell()->tag() == radio::SYMBOL;
}

inline bool JSLayout::IsObject() const {
  return IsCell() && cell()->tag() == radio::OBJECT;
}

inline bool JSLayout::IsReference() const {
  return IsCell() && cell()->tag() == radio::REFERENCE;
}

inline bool JSLayout::IsEnvironment() const {
  return IsCell() && cell()->tag() == radio::ENVIRONMENT;
}

inline bool JSLayout::IsOtherCell() const {
  return IsCell() && cell()->tag() == radio::POINTER;
}

inline bool JSLayout::IsPrimitive() const {
  return IsNumber() || IsString() || IsBoolean() || IsSymbol();
}

inline JSReference* JSLayout::reference() const {
  assert(IsReference());
  return static_cast<JSReference*>(value_.cell_);
}

inline JSEnv* JSLayout::environment() const {
  assert(IsEnvironment());
  return static_cast<JSEnv*>(value_.cell_);
}

inline JSString* JSLayout::string() const {
  assert(IsString());
  return static_cast<JSString*>(value_.cell_);
}

inline JSSymbol* JSLayout::symbol() const {
  assert(IsSymbol());
  return static_cast<JSSymbol*>(value_.cell_);
}

inline JSObject* JSLayout::object() const {
  assert(IsObject());
  return static_cast<JSObject*>(value_.cell_);
}

inline bool JSLayout::boolean() const {
  assert(IsBoolean());
  return value_.bytes_ & 0x1;
}

inline int32_t JSLayout::int32() const {
  assert(IsInt32());
  return static_cast<int32_t>(value_.bytes_);
}

inline double JSLayout::number() const {
  assert(IsNumber());
  if (IsInt32()) {
    return int32();
  } else {
    return core::BitCast<double>(
        value_.bytes_ - detail::jsval64::kDoubleOffset);
  }
}

inline void JSLayout::set_value_int32(int32_t val) {
  // NUMBERMASK | 32bit pattern of int32_t
  value_.bytes_ = detail::jsval64::kNumberMask | static_cast<uint32_t>(val);
}

inline void JSLayout::set_value_uint32(uint32_t val) {
  if (static_cast<int32_t>(val) < 0) {  // LSB is 1
    value_.bytes_ =
        core::BitCast<uint64_t>(
            static_cast<double>(val)) + detail::jsval64::kDoubleOffset;
  } else {
    set_value_int32(static_cast<int32_t>(val));
  }
}

inline void JSLayout::set_value(double val) {
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

inline void JSLayout::set_value_cell(radio::Cell* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

inline void JSLayout::set_value(JSObject* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

inline void JSLayout::set_value(JSString* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

inline void JSLayout::set_value(JSSymbol* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

inline void JSLayout::set_value(JSReference* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

inline void JSLayout::set_value(JSEnv* val) {
  value_.cell_ = static_cast<radio::Cell*>(val);
}

inline void JSLayout::set_value(detail::JSTrueType val) {
  value_.bytes_ = detail::jsval64::kTrue;
}

inline void JSLayout::set_value(detail::JSFalseType val) {
  value_.bytes_ = detail::jsval64::kFalse;
}

inline void JSLayout::set_value(detail::JSNaNType val) {
  value_.bytes_ = detail::jsval64::kNaN;
}

inline void JSLayout::set_null() {
  value_.bytes_ = detail::jsval64::kNull;
}

inline void JSLayout::set_undefined() {
  value_.bytes_ = detail::jsval64::kUndefined;
}

inline void JSLayout::set_empty() {
  value_.bytes_ = detail::jsval64::kEmpty;
}

inline bool JSLayout::SameValue(this_type lhs, this_type rhs) {
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
    const double lhsn = lhs.number();
    const double rhsn = rhs.number();
    // because lhs and rhs are double, so +0 and -0 cannot come here.
    if (lhsn == rhsn) {
      return true;
    }
    return core::math::IsNaN(lhsn) && core::math::IsNaN(rhsn);
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

inline bool JSLayout::StrictEqual(this_type lhs, this_type rhs) {
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

inline std::size_t JSLayout::Hasher::operator()(JSLayout val) const {
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

  // Cover symbol, object and other cells.
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
static const uint32_t kSymbolTag      = 0xfffffffb;
static const uint32_t kObjectTag      = 0xfffffffa;  // cell range start
static const uint32_t kEmptyTag       = 0xfffffff9;
static const uint32_t kUndefinedTag   = 0xfffffff8;
static const uint32_t kNullTag        = 0xfffffff7;
static const uint32_t kBoolTag        = 0xfffffff6;
static const uint32_t kNumberTag      = 0xfffffff5;
static const uint32_t kInt32Tag       = 0xfffffff4;

inline bool InPtrRange(uint32_t tag) {
  return kObjectTag <= tag;
}

inline uint32_t GetType(const JSLayout& val) {
  return val.IsNumber() ? kNumberTag : val.Layout().struct_.tag_;
}

} }  // namespace detail::jsval32

inline bool JSLayout::IsEmpty() const {
  return value_.struct_.tag_ == detail::jsval32::kEmptyTag;
}

inline bool JSLayout::IsUndefined() const {
  return value_.struct_.tag_ == detail::jsval32::kUndefinedTag;
}

inline bool JSLayout::IsNull() const {
  return value_.struct_.tag_ == detail::jsval32::kNullTag;
}

inline bool JSLayout::IsNullOrUndefined() const {
  return IsNull() || IsUndefined();
}

inline bool JSLayout::IsBoolean() const {
  return value_.struct_.tag_ == detail::jsval32::kBoolTag;
}

inline bool JSLayout::IsString() const {
  return value_.struct_.tag_ == detail::jsval32::kStringTag;
}

inline bool JSLayout::IsSymbol() const {
  return value_.struct_.tag_ == detail::jsval32::kSymbolTag;
}

inline bool JSLayout::IsObject() const {
  return value_.struct_.tag_ == detail::jsval32::kObjectTag;
}

inline bool JSLayout::IsInt32() const {
  return value_.struct_.tag_ == detail::jsval32::kInt32Tag;
}

inline bool JSLayout::IsNumber() const {
  return value_.struct_.tag_ < detail::jsval32::kNumberTag;
}

inline bool JSLayout::IsReference() const {
  return value_.struct_.tag_ == detail::jsval32::kReferenceTag;
}

inline bool JSLayout::IsEnvironment() const {
  return value_.struct_.tag_ == detail::jsval32::kEnvironmentTag;
}

inline bool JSLayout::IsOtherCell() const {
  return value_.struct_.tag_ == detail::jsval32::kOtherCellTag;
}

inline bool JSLayout::IsCell() const {
  return detail::jsval32::InPtrRange(value_.struct_.tag_);
}

inline bool JSLayout::IsPrimitive() const {
  return IsNumber() || IsString() || IsBoolean() || IsSymbol();
}

inline JSReference* JSLayout::reference() const {
  assert(IsReference());
  return value_.struct_.payload_.reference_;
}

inline JSEnv* JSLayout::environment() const {
  assert(IsEnvironment());
  return value_.struct_.payload_.environment_;
}

inline JSString* JSLayout::string() const {
  assert(IsString());
  return value_.struct_.payload_.string_;
}

inline JSSymbol* JSLayout::symbol() const {
  assert(IsSymbol());
  return value_.struct_.payload_.symbol_;
}

inline JSObject* JSLayout::object() const {
  assert(IsObject());
  return value_.struct_.payload_.object_;
}

inline radio::Cell* JSLayout::cell() const {
  assert(IsCell());
  return value_.struct_.payload_.cell_;
}

inline bool JSLayout::boolean() const {
  assert(IsBoolean());
  return value_.struct_.payload_.boolean_;
}

inline int32_t JSLayout::int32() const {
  assert(IsInt32());
  return value_.struct_.payload_.int32_;
}

inline double JSLayout::number() const {
  assert(IsNumber());
  if (IsInt32()) {
    return int32();
  } else {
    return value_.number_.as_;
  }
}

inline void JSLayout::set_value_cell(radio::Cell* ptr) {
  value_.struct_.payload_.cell_ = ptr;
  value_.struct_.tag_ = detail::jsval32::kOtherCellTag;
}

inline void JSLayout::set_value_int32(int32_t val) {
  value_.struct_.payload_.int32_ = val;
  value_.struct_.tag_ = detail::jsval32::kInt32Tag;
}

inline void JSLayout::set_value_uint32(uint32_t val) {
  if (static_cast<int32_t>(val) < 0) {  // LSB is 1
    value_.number_.as_ = val;
  } else {
    set_value_int32(static_cast<int32_t>(val));
  }
}

inline void JSLayout::set_value(double val) {
  const int32_t i = static_cast<int32_t>(val);
  if (val != i || (!i && core::math::Signbit(val))) {
    // this value is not represented by int32_t
#if defined(IV_CPU_X64)
    value_.number_.as_ = val;
#else
    value_.number_.as_ = (val == val) ? val : core::kNaN;
#endif  // defined(IV_CPU_X64)
  } else {
    set_value_int32(i);
  }
}

inline void JSLayout::set_value(JSObject* val) {
  value_.struct_.payload_.object_ = val;
  value_.struct_.tag_ = detail::jsval32::kObjectTag;
}

inline void JSLayout::set_value(JSString* val) {
  value_.struct_.payload_.string_ = val;
  value_.struct_.tag_ = detail::jsval32::kStringTag;
}

inline void JSLayout::set_value(JSSymbol* val) {
  value_.struct_.payload_.symbol_ = val;
  value_.struct_.tag_ = detail::jsval32::kSymbolTag;
}

inline void JSLayout::set_value(JSReference* ref) {
  value_.struct_.payload_.reference_ = ref;
  value_.struct_.tag_ = detail::jsval32::kReferenceTag;
}

inline void JSLayout::set_value(JSEnv* ref) {
  value_.struct_.payload_.environment_ = ref;
  value_.struct_.tag_ = detail::jsval32::kEnvironmentTag;
}

inline void JSLayout::set_value(detail::JSTrueType val) {
  value_.struct_.payload_.boolean_ = true;
  value_.struct_.tag_ = detail::jsval32::kBoolTag;
}

inline void JSLayout::set_value(detail::JSFalseType val) {
  value_.struct_.payload_.boolean_ = false;
  value_.struct_.tag_ = detail::jsval32::kBoolTag;
}

inline void JSLayout::set_value(detail::JSNaNType val) {
  value_.number_.as_ = core::kNaN;
}

inline void JSLayout::set_null() {
  value_.struct_.tag_ = detail::jsval32::kNullTag;
}

inline void JSLayout::set_undefined() {
  value_.struct_.tag_ = detail::jsval32::kUndefinedTag;
}

inline void JSLayout::set_empty() {
  value_.struct_.tag_ = detail::jsval32::kEmptyTag;
}

inline bool JSLayout::SameValue(this_type lhs, this_type rhs) {
  if (detail::jsval32::GetType(lhs) != detail::jsval32::GetType(rhs)) {
    return false;
  }
  if (lhs.IsNullOrUndefined()) {
    return true;
  }
  if (lhs.IsNumber()) {
    const double lhsn = lhs.number();
    const double rhsn = rhs.number();
    if (lhsn == rhsn) {
      return core::math::Signbit(lhsn) == core::math::Signbit(rhsn);
    }
    return core::math::IsNaN(lhsn) && core::math::IsNaN(rhsn);
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
  if (lhs.IsSymbol()) {
    return lhs.symbol() == rhs.symbol();
  }
  assert(!lhs.IsEmpty());
  return false;
}

inline bool JSLayout::StrictEqual(this_type lhs, this_type rhs) {
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
  if (lhs.IsSymbol()) {
    return lhs.symbol() == rhs.symbol();
  }
  return false;
}

inline std::size_t JSLayout::Hasher::operator()(JSLayout val) const {
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

  // Cover symbol, object and other cells.
  if (val.IsCell()) {
    return std::hash<radio::Cell*>()(val.cell());
  }

  if (val.IsBoolean()) {
    return std::hash<uint32_t>()(val.boolean());
  }

  return std::hash<uint32_t>()(detail::jsval32::GetType(val));
}

#endif  // 32 or 64 bit JSLayout ifdef

inline JSString* JSLayout::TypeOf(Context* ctx) const {
  // This method must not allocate new object
  if (IsObject()) {
    if (object()->IsCallable()) {
      return ctx->global_data()->string_function();
    }
    return ctx->global_data()->string_object();
  } else if (IsNumber()) {
    return ctx->global_data()->string_number();
  } else if (IsString()) {
    return ctx->global_data()->string_string();
  } else if (IsBoolean()) {
    return ctx->global_data()->string_boolean();
  } else if (IsUndefined()) {
    return ctx->global_data()->string_undefined();
  } else if (IsNull()) {
    return ctx->global_data()->string_object();
  } else {
    assert(IsSymbol());
    return ctx->global_data()->string_symbol();
  }
}

inline JSObject* JSLayout::GetPrimitiveProto(Context* ctx) const {
  assert(IsPrimitive());
  if (IsString()) {
    return ctx->global_data()->string_prototype();
  } else if (IsNumber()) {
    return ctx->global_data()->number_prototype();
  } else if (IsBoolean()) {
    return ctx->global_data()->boolean_prototype();
  } else {
    assert(IsSymbol());
    return ctx->global_data()->symbol_prototype();
  }
}

inline JSObject* JSLayout::ToObject(Context* ctx, Error* e) const {
  if (IsNullOrUndefined()) {
    e->Report(Error::Type, "ToObject to null or undefined");
    return nullptr;
  } else {
    return ToObject(ctx);
  }
}

inline JSObject* JSLayout::ToObject(Context* ctx) const {
  assert(!IsNullOrUndefined());
  if (IsObject()) {
    return object();
  } else if (IsNumber()) {
    return JSNumberObject::New(ctx, number());
  } else if (IsString()) {
    return JSStringObject::New(ctx, string());
  } else if (IsBoolean()) {
    return JSBooleanObject::New(ctx, boolean());
  } else {
    assert(IsSymbol());
    return JSSymbolObject::New(ctx, symbol());
  }
}

inline JSString* JSLayout::ToString(Context* ctx, Error* e) const {
  if (IsString()) {
    return string();
  } else if (IsInt32()) {
    // int32 short cut
    std::array<char, 15> buffer;
    char* end = core::Int32ToString(int32(), buffer.data());
    return JSString::New(ctx, buffer.data(), end, true, e);
  } else if (IsNumber()) {
    std::array<char, 80> buffer;
    const char* const str =
        core::DoubleToCString(number(), buffer.data(), buffer.size());
    return JSString::NewAsciiString(ctx, str, e);
  } else if (IsBoolean()) {
    return boolean() ?
        ctx->global_data()->string_true() : ctx->global_data()->string_false();
  } else if (IsNull()) {
    return ctx->global_data()->string_null();
  } else if (IsUndefined()) {
    return ctx->global_data()->string_undefined();
  } else if (IsSymbol()) {
    e->Report(Error::Type, "Cannot perform ToString operation on Symbols");
    return nullptr;
  } else {
    assert(IsObject());
    JSLayout prim =
        object()->DefaultValue(ctx, Hint::STRING, IV_LV5_ERROR_WITH(e, nullptr));
    return prim.ToString(ctx, e);
  }
}

inline Symbol JSLayout::ToSymbol(Context* ctx, Error* e) const {
  uint32_t index;
  if (GetUInt32(&index)) {
    return symbol::MakeSymbolFromIndex(index);
  }

  if (IsString()) {
    return ctx->Intern(string());
  }

  if (IsNumber()) {
    // int32 short cut
    if (IsInt32()) {
      std::array<char, 15> buffer;
      char* end = core::Int32ToString(int32(), buffer.data());
      return ctx->Intern(
          core::StringPiece(buffer.data(),
                            std::distance(buffer.data(), end)));
    } else {
      std::array<char, 80> buffer;
      const char* const str =
          core::DoubleToCString(number(), buffer.data(), buffer.size());
      return ctx->Intern(str);
    }
  }

  if (IsSymbol()) {
    return symbol()->symbol();
  }

  if (IsBoolean()) {
    return ctx->Intern((boolean() ? "true" : "false"));
  }

  if (IsNull()) {
    return symbol::null();
  }

  if (IsUndefined()) {
    return symbol::undefined();
  }
  assert(IsObject());
  JSObject* obj = object();

  const JSLayout prim =
      object()->DefaultValue(
          ctx, Hint::STRING,
          IV_LV5_ERROR_WITH(e, symbol::kDummySymbol));
  return prim.ToSymbol(ctx, e);
}

inline core::UString JSLayout::ToUString(Context* ctx, Error* e) const {
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
  } else if (IsSymbol()) {
    e->Report(Error::Type, "Cannot perform ToString operation on Symbols");
    return core::UString();
  } else {
    assert(IsObject());
    const JSLayout prim =
        object()->DefaultValue(
            ctx, Hint::STRING,
            IV_LV5_ERROR_WITH(e, core::UString()));
    return prim.ToUString(ctx, e);
  }
}

inline double JSLayout::ToNumber(Context* ctx, Error* e) const {
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
  } else if (IsSymbol()) {
    return core::kNaN;
  } else {
    assert(IsObject());
    JSLayout prim =
        object()->DefaultValue(ctx, Hint::NUMBER, IV_LV5_ERROR_WITH(e, 0.0));
    return prim.ToNumber(ctx, e);
  }
}

inline double JSLayout::ToInteger(Context* ctx, Error* e) const {
  return core::DoubleToInteger(ToNumber(ctx, e));
}

inline JSVal JSVal::ToNumberValue(Context* ctx, Error* e) const {
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
  } else if (IsSymbol()) {
    return core::kNaN;
  } else {
    assert(IsObject());
    const JSVal prim =
        object()->DefaultValue(ctx, Hint::NUMBER,
                               IV_LV5_ERROR_WITH(e, JSVal::Int32(0)));
    return prim.ToNumber(ctx, e);
  }
}

inline bool JSLayout::ToBoolean() const {
  if (IsNumber()) {
    const double num = number();
    return num != 0 && !core::math::IsNaN(num);
  } else if (IsString()) {
    return !string()->empty();
  } else if (IsNullOrUndefined()) {
    return false;
  } else if (IsBoolean()) {
    return boolean();
  } else if (IsSymbol()) {
    return true;
  } else {
    assert(!IsEmpty());
    return true;
  }
}

inline JSVal JSVal::ToPrimitive(Context* ctx,
                                Hint::Object hint, Error* e) const {
  if (IsObject()) {
    return object()->DefaultValue(ctx, hint, e);
  } else {
    assert(!IsEnvironment() && !IsReference() && !IsEmpty());
    return *this;
  }
}

inline int32_t JSLayout::ToInt32(Context* ctx, Error* e) const {
  if (IsInt32()) {
    return int32();
  } else {
    return core::DoubleToInt32(ToNumber(ctx, e));
  }
}

inline uint32_t JSLayout::ToUInt32(Context* ctx, Error* e) const {
  if (IsInt32() && int32() >= 0) {
    return static_cast<uint32_t>(int32());
  } else {
    return core::DoubleToUInt32(ToNumber(ctx, e));
  }
}

inline uint32_t JSLayout::GetUInt32() const {
  assert(IsNumber());
  uint32_t val = 0;  // make gcc happy
  GetUInt32(&val);
  return val;
}

inline bool JSLayout::GetUInt32(uint32_t* result) const {
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

inline bool JSLayout::IsCallable() const {
  return IsObject() && object()->IsCallable();
}

inline void JSLayout::CheckObjectCoercible(Error* e) const {
  assert(!IsEnvironment() && !IsReference() && !IsEmpty());
  if (IsNullOrUndefined()) {
    e->Report(Error::Type, "null or undefined has no properties");
  }
}

#define CHECK IV_LV5_ERROR_WITH(e, false)
inline bool JSVal::AbstractEqual(Context* ctx,
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
    if (lhs.IsSymbol() && rhs.IsSymbol()) {
      return lhs.symbol() == rhs.symbol();
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
inline CompareResult JSVal::Compare(Context* ctx,
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

inline JSVal JSVal::GetSlot(Context* ctx,
                            Symbol name, Slot* slot, Error* e) const {
  if (!IsObject()) {
    JSObject* proto = nullptr;
    if (IsNullOrUndefined()) {
      e->Report(Error::Type, "null or undefined has no properties");
      return JSEmpty;
    }

    assert(IsPrimitive());
    if (IsString()) {
      JSString* str = string();
      if (name == symbol::length()) {
        slot->set(
            JSVal::UInt32(str->size()),
            Attributes::String::Length(),
            str);
        return slot->value();
      }

      if (symbol::IsArrayIndexSymbol(name)) {
        const uint32_t index = symbol::GetIndexFromSymbol(name);
        if (index < str->size()) {
          slot->set(
              JSString::NewSingle(ctx, str->At(index)),
              Attributes::String::Indexed(),
              str);
          return slot->value();
        }
      }
    }
    proto = GetPrimitiveProto(ctx);
    if (proto->GetPropertySlot(ctx, name, slot)) {
      return slot->Get(ctx, *this, e);
    }
    return JSUndefined;
  }
  return object()->GetSlot(ctx, name, slot, e);
}

inline bool JSVal::GetPropertySlot(Context* ctx,
                                   Symbol name, Slot* slot, Error* e) const {
  JSObject* obj = nullptr;
  if (!IsObject()) {
    if (IsNullOrUndefined()) {
      e->Report(Error::Type, "null or undefined has no properties");
      return false;
    }

    assert(IsPrimitive());
    if (IsString()) {
      JSString* str = string();
      if (name == symbol::length()) {
        slot->set(
            JSVal::UInt32(str->size()),
            Attributes::String::Length(),
            str);
        return true;
      }

      if (symbol::IsArrayIndexSymbol(name)) {
        const uint32_t index = symbol::GetIndexFromSymbol(name);
        if (index < str->size()) {
          slot->set(
              JSString::NewSingle(ctx, str->At(index)),
              Attributes::String::Indexed(),
              str);
          return true;
        }
      }
    }
    obj = GetPrimitiveProto(ctx);
  } else {
    obj = object();
  }

  return obj->GetPropertySlot(ctx, name, slot);
}

inline bool JSLayout::BitEqual(this_type lhs, this_type rhs) {
#if defined(IV_OS_SOLARIS) && defined(IV_64)
  return
      lhs.value_.bytes_.high == rhs.value_.bytes_.high &&
      lhs.value_.bytes_.low == rhs.value_.bytes_.low;
#else
  return lhs.value_.bytes_ == rhs.value_.bytes_;
#endif
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSVAL_H_
