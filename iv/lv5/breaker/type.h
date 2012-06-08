#ifndef IV_LV5_BREAKER_TYPE_H_
#define IV_LV5_BREAKER_TYPE_H_
#include <iv/lv5/jsval_fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

// Type information for specialization.
// This information purges
//   constant loading
//   type checking in breaker::Compiler
// See also
// http://wingolog.org/archives/2011/07/05/v8-a-tale-of-two-compilers
typedef uint32_t TypeValue;
static const TypeValue TYPE_UNINITIALIZED          = 0x00000000;
static const TypeValue TYPE_STRING                 = 0x00000001;
static const TypeValue TYPE_UNDEFINED              = 0x00000002;
static const TypeValue TYPE_NULL                   = 0x00000004;
static const TypeValue TYPE_BOOLEAN                = 0x00000008;
static const TypeValue TYPE_INT32                  = 0x00000010;
static const TypeValue TYPE_NUMBER_DOUBLE          = 0x00000020;
static const TypeValue TYPE_NUMBER_MASK            = 0x00000030;
static const TypeValue TYPE_ARRAY                  = 0x00010000;
static const TypeValue TYPE_FUNCTION               = 0x00020000;
static const TypeValue TYPE_ARGUMENTS              = 0x00040000;
static const TypeValue TYPE_OBJECT_GENERIC         = 0x00080000;
static const TypeValue TYPE_OBJECT_MASK            = 0x00FF0000;
static const TypeValue TYPE_EMPTY                  = 0x01000000;
static const TypeValue TYPE_UNKNOWN                = 0xFFFFFFFF;

class Type {
 public:
  explicit Type(JSVal value)
    : type_(TYPE_UNINITIALIZED) {
    if (value.IsNumber()) {
      if (value.IsInt32()) {
        type_ = TYPE_INT32;
      } else {
        type_ = TYPE_NUMBER_DOUBLE;
      }
    } else if (value.IsString()) {
      type_ = TYPE_STRING;
    } else if (value.IsUndefined()) {
      type_ = TYPE_UNDEFINED;
    } else if (value.IsNull()) {
      type_ = TYPE_NULL;
    } else if (value.IsBoolean()) {
      type_ = TYPE_BOOLEAN;
    } else if (value.IsObject()) {
      if (value.object()->IsClass<Class::Array>()) {
        type_ = TYPE_ARRAY;
      } else if (value.object()->IsClass<Class::Arguments>()) {
        type_ = TYPE_ARGUMENTS;
      } else {
        type_ = TYPE_OBJECT_GENERIC;
      }
    } else if (value.IsEmpty()) {
      type_ = TYPE_EMPTY;
    } else {
      type_ = TYPE_UNKNOWN;
    }
  }

  explicit Type(TypeValue value)
    : type_(value) { }

  bool IsInt32() const { return type_ == TYPE_INT32; }

  bool IsNumber() const {
    return (type_ & TYPE_NUMBER_MASK) && !(type_ & ~TYPE_NUMBER_MASK);
  }

  bool IsString() const { return type_ == TYPE_STRING; }

  bool IsNotString() const {
    return !IsString() && !IsUnknown();
  }

  bool IsBoolean() const { return type_ == TYPE_BOOLEAN; }

  bool IsUndefined() const { return type_ == TYPE_UNDEFINED; }

  bool IsNull() const { return type_ == TYPE_NULL; }

  bool IsUnknown() const { return type_ == TYPE_UNKNOWN; }

  friend Type operator|(const Type& lhs, const Type& rhs) {
    return Type(lhs.type_ | rhs.type_);
  }

  static Type Unknown() {
    return Type(TYPE_UNKNOWN);
  }

  static Type String() {
    return Type(TYPE_STRING);
  }

  static Type Number() {
    return Type(TYPE_NUMBER_MASK);
  }

  static Type Int32() {
    return Type(TYPE_INT32);
  }

  static Type Boolean() {
    return Type(TYPE_BOOLEAN);
  }

  static Type Array() {
    return Type(TYPE_ARRAY);
  }

  static Type Arguments() {
    return Type(TYPE_ARGUMENTS);
  }


  static Type Function() {
    return Type(TYPE_FUNCTION);
  }

  static Type Object() {
    return Type(TYPE_OBJECT_GENERIC);
  }
 private:
  TypeValue type_;
};

class TypeEntry {
 public:
  TypeEntry()
    : type_(Type::Unknown()), constant_(JSEmpty) { }
  explicit TypeEntry(Type type)
    : type_(type), constant_(JSEmpty) { }
  explicit TypeEntry(JSVal constant)
    : type_(Type(constant)), constant_(constant) { }

  Type type() const {
    return type_;
  }

  bool IsConstant() const {
    return !constant_.IsEmpty();
  }

  JSVal constant() const { return constant_; }

  static TypeEntry Add(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.type().IsString() || rhs.type().IsString()) {
      return TypeEntry(Type::String());
    } else if (lhs.type().IsNotString() && rhs.type().IsNotString()) {
      return TypeEntry(Type::Number());
    } else {
      // merged
      return TypeEntry(Type::Number() | Type::String());
    }
  }

  static TypeEntry Subtract(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(JSVal(lhs.constant().number() - rhs.constant().number()));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Multiply(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(JSVal(lhs.constant().number() * rhs.constant().number()));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Divide(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(JSVal(lhs.constant().number() / rhs.constant().number()));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Modulo(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Number());
  }

  static TypeEntry Lshift(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) <<
              (core::DoubleToInt32(lhs.constant().number()) & 0x1F)));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry Rshift(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) >>
              (core::DoubleToInt32(lhs.constant().number()) & 0x1F)));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry RshiftLogical(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::UInt32(
              core::DoubleToUInt32(lhs.constant().number()) >>
              (core::DoubleToInt32(lhs.constant().number()) & 0x1F)));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry LT(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry LTE(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry GT(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry GTE(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry Instanceof(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry In(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry Equal(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry StrictEqual(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && rhs.IsConstant()) {
      return TypeEntry(
          JSVal::Bool(JSVal::StrictEqual(lhs.constant(), rhs.constant())));
    }
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry NotEqual(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry StrictNotEqual(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && rhs.IsConstant()) {
      return TypeEntry(
          JSVal::Bool(!JSVal::StrictEqual(lhs.constant(), rhs.constant())));
    }
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry BitwiseAnd(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) &
              core::DoubleToInt32(lhs.constant().number())));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry BitwiseOr(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) |
              core::DoubleToInt32(lhs.constant().number())));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry BitwiseXor(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) ^
              core::DoubleToInt32(lhs.constant().number())));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry Positive(const TypeEntry& src) {
    if (src.IsConstant() && src.constant().IsNumber()) {
      return src;
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Negative(const TypeEntry& src) {
    if (src.IsConstant() && src.constant().IsNumber()) {
      return TypeEntry(-src.constant().number());
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Not(const TypeEntry& src) {
    if (src.IsConstant()) {
      return TypeEntry(JSVal::Bool(!src.constant().ToBoolean()));
    }
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry BitwiseNot(const TypeEntry& src) {
    if (src.IsConstant() && src.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(~core::DoubleToInt32(src.constant().number())));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry ToPrimitiveAndToString(const TypeEntry& src) {
    if (src.type().IsString()) {
      return src;
    }
    return TypeEntry(Type::String());
  }

  static TypeEntry Increment(const TypeEntry& src) {
    if (src.IsConstant()) {
      if (src.constant().IsNumber()) {
        return TypeEntry(JSVal(src.constant().number() + 1));
      }
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Decrement(const TypeEntry& src) {
    if (src.IsConstant()) {
      if (src.constant().IsNumber()) {
        return TypeEntry(JSVal(src.constant().number() - 1));
      }
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry ToNumber(const TypeEntry& src) {
    if (src.IsConstant() && src.constant().IsNumber()) {
      return src;
    }
    return TypeEntry(Type::Number());
  }

 private:
  Type type_;
  JSVal constant_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TYPE_H_
