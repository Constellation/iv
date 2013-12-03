#ifndef IV_LV5_JSVAL_FWD_H_
#define IV_LV5_JSVAL_FWD_H_
#include <cmath>
#include <algorithm>
#include <type_traits>
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/byteorder.h>
#include <iv/platform.h>
#include <iv/platform_math.h>
#include <iv/canonicalized_nan.h>
#include <iv/conversions.h>
#include <iv/none.h>
#include <iv/utils.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/hint.h>
#include <iv/lv5/symbol.h>

namespace iv {
namespace lv5 {
namespace radio {

class Cell;

}  // namespace radio

class Error;
class JSEnv;
class Context;
class JSReference;
class JSEnv;
class JSObject;
class JSString;
class JSSymbol;
class Slot;

namespace detail {
template<std::size_t PointerSize, bool IsLittle>
struct Layout;

template<>
struct Layout<4, true> {
  union {
    uint64_t bytes_;
    struct {
      double as_;
    } number_;
    struct {
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSSymbol* symbol_;
        JSReference* reference_;
        JSEnv* environment_;
        int32_t int32_;
        radio::Cell* cell_;
      } payload_;
      uint32_t tag_;
    } struct_;
  };
};

template<>
struct Layout<4, false> {
  union {
    uint64_t bytes_;
    struct {
      double as_;
    } number_;
    struct {
      uint32_t tag_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSSymbol* symbol_;
        JSReference* reference_;
        JSEnv* environment_;
        int32_t int32_;
        radio::Cell* cell_;
      } payload_;
    } struct_;
  };
};

#if defined(IV_OS_SOLARIS)

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
        JSSymbol* symbol_;
        JSReference* reference_;
        JSEnv* environment_;
        JSVal* jsvalref_;
        int32_t int32_;
        radio::Cell* cell_;
      } payload_;
    } struct_;
    struct {
      uint64_t low;
      uint64_t high;
    } bytes_;
  };
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
        JSSymbol* symbol_;
        JSReference* reference_;
        JSEnv* environment_;
        JSVal* jsvalref_;
        int32_t int32_;
        radio::Cell* cell_;
      } payload_;
    } struct_;
    struct {
      uint64_t high;
      uint64_t low;
    } bytes_;
  };
};

#else

template<bool IsLittle>
struct Layout<8, IsLittle> {
  union {
    uint64_t bytes_;
    radio::Cell* cell_;
  };
};

#endif

struct JSTrueType { };
struct JSFalseType { };
struct JSNullType { };
struct JSUndefinedType { };
struct JSEmptyType { };
struct JSNaNType { };

struct Int32Tag { };
struct UInt32Tag { };
struct UInt16Tag { };
struct OtherCellTag { };

}  // namespace detail

static const detail::JSTrueType JSTrue = {};
static const detail::JSFalseType JSFalse = {};
static const detail::JSNullType JSNull = {};
static const detail::JSUndefinedType JSUndefined = {};
static const detail::JSEmptyType JSEmpty = {};
static const detail::JSNaNType JSNaN = {};

enum CompareResult {
  CMP_FALSE = 0,
  CMP_TRUE = 1,
  CMP_UNDEFINED = 2
};

class JSLayout {
 public:
  typedef JSLayout this_type;
  typedef detail::Layout<
      core::Size::kPointerSize,
      core::kLittleEndian> value_type;

  // JSLayout Hasher is used by SameValue algorithm
  struct Hasher {
    std::size_t operator()(this_type val) const;
  };

  friend struct Hasher;

  struct SameValueEqualer {
    bool operator()(this_type lhs, this_type rhs) const {
      return this_type::SameValue(lhs, rhs);
    }
  };

  struct StrictEqualer {
    bool operator()(this_type lhs, this_type rhs) const {
      return this_type::StrictEqual(lhs, rhs);
    }
  };

  JSString* TypeOf(Context* ctx) const;

  JSObject* GetPrimitiveProto(Context* ctx) const;

  JSObject* ToObject(Context* ctx, Error* e) const;

  // Assertion failed when this is null or undefined
  JSObject* ToObject(Context* ctx) const;

  JSString* ToString(Context* ctx, Error* e) const;

  Symbol ToSymbol(Context* ctx, Error* e) const;

  core::UString ToUString(Context* ctx, Error* e) const;

  double ToNumber(Context* ctx, Error* e) const;

  double ToInteger(Context* ctx, Error* e) const;

  int32_t ToInt32(Context* ctx, Error* e) const;

  uint32_t ToUInt32(Context* ctx, Error* e) const;

  uint32_t GetUInt32() const;

  bool GetUInt32(uint32_t* result) const;

  bool IsCallable() const;

  void CheckObjectCoercible(Error* e) const;

  bool ToBoolean() const;

  // 32 / 64bit system separately
  bool IsEmpty() const;

  bool IsUndefined() const;

  bool IsNull() const;

  bool IsNullOrUndefined() const;

  bool IsBoolean() const;

  bool IsInt32() const;

  bool IsNumber() const;

  bool IsCell() const;

  bool IsString() const;

  bool IsSymbol() const;

  bool IsObject() const;

  bool IsReference() const;

  bool IsEnvironment() const;

  bool IsOtherCell() const;

  bool IsPrimitive() const;

  JSReference* reference() const;

  JSEnv* environment() const;

  JSString* string() const;

  JSSymbol* symbol() const;

  JSObject* object() const;

  bool boolean() const;

  int32_t int32() const;

  double number() const;

  radio::Cell* cell() const;

  void set_value_int32(int32_t val);

  void set_value_uint32(uint32_t val);

  void set_value(double val);

  void set_value_cell(radio::Cell* val);

  void set_value(JSObject* val);

  void set_value(JSString* val);

  void set_value(JSSymbol* val);

  void set_value(JSReference* val);

  void set_value(JSEnv* val);

  void set_value(detail::JSTrueType val);

  void set_value(detail::JSFalseType val);

  void set_value(detail::JSNaNType val);

  void set_null();

  void set_undefined();

  void set_empty();

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

  static bool SameValue(this_type lhs, this_type rhs);

  static bool StrictEqual(this_type lhs, this_type rhs);

  static bool BitEqual(this_type lhs, this_type rhs);

  value_type value_;
};

#if defined(IV_COMPILER_GCC) && (IV_COMPILER_GCC > 40300)
static_assert(std::is_pod<JSLayout>::value, "JSLayout should be POD");
#endif

class JSVal : public JSLayout {
 public:
  typedef JSVal this_type;

  JSVal() {
    set_undefined();
  }

  JSVal(double val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsNumber());
  }

  JSVal(JSObject* val)  // NOLINT
    : JSLayout() {
    set_value(val);
  }

  JSVal(JSString* val)  // NOLINT
    : JSLayout() {
    set_value(val);
  }

  JSVal(JSSymbol* val)  // NOLINT
    : JSLayout() {
    set_value(val);
  }

  JSVal(JSReference* val)  // NOLINT
    : JSLayout() {
    set_value(val);
  }

  JSVal(JSEnv* val)  // NOLINT
    : JSLayout() {
    set_value(val);
  }

  JSVal(detail::JSTrueType val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsBoolean());
  }

  JSVal(detail::JSFalseType val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsBoolean());
  }

  JSVal(detail::JSNullType val)  // NOLINT
    : JSLayout() {
    set_null();
    assert(IsNull());
  }

  JSVal(detail::JSUndefinedType val)  // NOLINT
    : JSLayout() {
    set_undefined();
    assert(IsUndefined());
  }

  JSVal(detail::JSEmptyType val)  // NOLINT
    : JSLayout() {
    set_empty();
    assert(IsEmpty());
  }

  JSVal(detail::JSNaNType val)  // NOLINT
    : JSLayout() {
    set_value(val);
    assert(IsNumber());
  }

  JSVal(value_type val)  // NOLINT
    : JSLayout() {
    value_ = val;
  }

  JSVal(JSLayout layout)  // NOLINT
    : JSLayout(layout) {
  }

  // prohibit like this,
  //
  //   JSVal val = true;
  //
  // if you want Boolean value, use JSTrue / JSFalse,
  //
  //   JSVal val = JSTrue;
  //
  // if bool constructor provided, following code doesn't raise compile error
  // (and any warning in GCC)
  //
  //   class A {
  //    public:
  //     A() { }
  //     explicit A(bool v) { }
  //     explicit A(A* v) { }
  //   };
  //
  //   int main() {
  //     const A a;
  //     const A b(&a);  // not compile error...
  //     return 0;
  //   }
  //
  //  so not provide implicit constructor with bool.
  template<typename T>
  JSVal(T val,
        typename std::enable_if<std::is_same<bool, T>::value>::type* = 0) {
    static_assert(!(std::is_same<bool, T>::value), "T should be bool");
  }

  JSVal ToPrimitive(Context* ctx, Hint::Object hint, Error* e) const;

  JSVal ToNumberValue(Context* ctx, Error* e) const;

  // type specified factory functions
  static inline JSVal Bool(bool val) {
    if (val) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  }

  template<typename T>
  static inline JSVal Signed(T val,
      typename std::enable_if<std::is_same<int32_t, T>::value ||
                              std::is_same<int16_t, T>::value ||
                              std::is_same<int8_t, T>::value>::type* = 0) {
    return JSVal(val, detail::Int32Tag());
  }

  template<typename T>
  static inline JSVal UnSigned(T val,
      typename std::enable_if<std::is_same<uint32_t, T>::value ||
                              std::is_same<uint16_t, T>::value ||
                              std::is_same<uint8_t, T>::value  ||
                              std::is_same<char16_t, T>::value ||
                              std::is_same<char32_t, T>::value>::type* = 0) {
    return JSVal(val, detail::UInt32Tag());
  }

  template<typename T>
  static inline JSVal UInt32(
      T val,
      typename std::enable_if<std::is_same<uint32_t, T>::value ||
                              std::is_same<char32_t, T>::value >::type* = 0) {
    return JSVal(val, detail::UInt32Tag());
  }

  template<typename T>
  static inline JSVal UInt16(
      T val,
      typename std::enable_if<std::is_same<uint16_t, T>::value ||
                              std::is_same<char16_t, T>::value >::type* = 0) {
    return JSVal(val, detail::UInt16Tag());
  }

  template<typename T>
  static inline JSVal Int32(
      T val,
      typename std::enable_if<std::is_same<int32_t, T>::value>::type* = 0) {
    return JSVal(val, detail::Int32Tag());
  }

  static inline JSVal Cell(radio::Cell* cell) {
    return JSVal(cell, detail::OtherCellTag());
  }

  static bool AbstractEqual(Context* ctx,
                            this_type lhs,
                            this_type rhs, Error* e);

  static inline bool SameValue(this_type lhs, this_type rhs) {
    return JSLayout::SameValue(lhs, rhs);
  }

  static inline bool StrictEqual(this_type lhs, this_type rhs) {
    return JSLayout::StrictEqual(lhs, rhs);
  }

  static inline bool BitEqual(this_type lhs, this_type rhs) {
    return JSLayout::BitEqual(lhs, rhs);
  }

  static inline CompareResult NumberCompare(double lhs, double rhs) {
    if (core::math::IsNaN(lhs) || core::math::IsNaN(rhs)) {
      return CMP_UNDEFINED;
    }
    if (lhs == rhs) {
      return CMP_FALSE;
    }
    return (lhs < rhs) ? CMP_TRUE : CMP_FALSE;
  }

  template<bool LeftFirst>
  static CompareResult Compare(Context* ctx,
                               this_type lhs,
                               this_type rhs, Error* e);

  bool GetPropertySlot(Context* ctx, Symbol name, Slot* slot, Error* e) const;

  JSVal GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e) const;

 private:
  JSVal(uint32_t val, detail::UInt32Tag dummy)
    : JSLayout() {
    set_value_uint32(val);
    assert(IsNumber());
  }

  JSVal(uint16_t val, detail::UInt16Tag dummy)
    : JSLayout() {
    set_value_int32(val);
    assert(IsInt32());
  }

  JSVal(int32_t val, detail::Int32Tag dummy)
    : JSLayout() {
    set_value_int32(val);
    assert(IsInt32());
  }

  JSVal(radio::Cell* val, detail::OtherCellTag dummy)
    : JSLayout() {
    set_value_cell(val);
  }
};

typedef GCVector<JSVal>::type JSVals;

} }  // namespace iv::lv5
#endif  // IV_LV5_JSVAL_FWD_H_
