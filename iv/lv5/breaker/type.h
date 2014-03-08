#ifndef IV_LV5_BREAKER_TYPE_H_
#define IV_LV5_BREAKER_TYPE_H_
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/breaker/context_fwd.h>
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

  // Always Int32
  bool IsInt32() const { return type_ == TYPE_INT32; }

  // Always not Int32
  // Number may be Int32, so purge it.
  bool IsNotInt32() const {
    return IsDouble() || IsNotNumber();
  }

  bool MaybeNumber() const {
    return (type_ & TYPE_NUMBER_MASK);
  }

  bool IsNumber() const {
    return MaybeNumber() && !(type_ & ~TYPE_NUMBER_MASK);
  }

  bool IsNotNumber() const {
    return !MaybeNumber() && !IsUnknown();
  }

  // Always Double
  bool IsDouble() const {
    return type_ == TYPE_NUMBER_DOUBLE;
  }

  bool IsString() const { return type_ == TYPE_STRING; }

  bool MaybeString() const { return type_ & TYPE_STRING; }

  bool IsNotString() const {
    return !MaybeString() && !IsUnknown();
  }

  bool IsBoolean() const { return type_ == TYPE_BOOLEAN; }

  bool IsUndefined() const { return type_ == TYPE_UNDEFINED; }

  bool MaybeUndefined() const {
    return type_ & TYPE_UNDEFINED;
  }

  bool IsNotUndefined() const {
    return !MaybeUndefined() && !IsUnknown();
  }

  bool IsNull() const { return type_ == TYPE_NULL; }

  bool MaybeNull() const {
    return type_ & TYPE_NULL;
  }

  bool IsNotNull() const {
    return !MaybeNull() && !IsUnknown();
  }

  bool IsUnknown() const { return type_ == TYPE_UNKNOWN; }

  bool IsArray() const { return type_ == TYPE_ARRAY; }

  bool IsFunction() const { return type_ == TYPE_FUNCTION; }

  bool IsObjectGeneric() const { return type_ == TYPE_OBJECT_GENERIC; }

  bool IsSomeObject() const {
    return IsArray() || IsFunction() || IsObjectGeneric();
  }

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

class TypeEntry : public Type {
 public:
  TypeEntry()
    : Type(Type::Unknown()), constant_(JSEmpty) { }
  explicit TypeEntry(Type type)
    : Type(type), constant_(JSEmpty) { }
  explicit TypeEntry(JSVal constant)
    : Type(Type(constant)), constant_(constant) { }

  bool IsConstant() const {
    return !constant_.IsEmpty();
  }

  bool IsConstantInt32() const {
    return IsConstant() && constant().IsInt32();
  }

  bool IsConstantDouble() const {
    return IsConstant() && constant().IsNumber() && !constant().IsInt32();
  }

  double ExtractConstantDouble() const {
    assert(IsConstantDouble());
    return constant().number();
  }

  int32_t ExtractConstantInt32() const {
    assert(IsConstantInt32());
    return constant().int32();
  }

  JSVal constant() const { return constant_; }

  static TypeEntry Add(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && rhs.IsConstant()) {
      if (lhs.constant().IsNumber() && rhs.constant().IsNumber()) {
        return TypeEntry(lhs.constant().number() + rhs.constant().number());
      }
    }

    if (lhs.IsString() || rhs.IsString()) {
      return TypeEntry(Type::String());
    } else if (lhs.IsNotString() && rhs.IsNotString()) {
      return TypeEntry(Type::Number());
    } else {
      // merged
      return TypeEntry(Type::Number() | Type::String());
    }
  }

  static TypeEntry Subtract(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal(lhs.constant().number() - rhs.constant().number()));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Multiply(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal(lhs.constant().number() * rhs.constant().number()));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Divide(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal(lhs.constant().number() / rhs.constant().number()));
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Modulo(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && rhs.IsConstant()) {
      if (lhs.constant().IsInt32() && rhs.constant().IsInt32()) {
        const int32_t left = lhs.constant().int32();
        const int32_t right = rhs.constant().int32();
        if (left >= 0 && right > 0) {
          return TypeEntry(JSVal::Int32(left % right));
        }
      }
      if (lhs.constant().IsNumber() && rhs.constant().IsNumber()) {
        return TypeEntry(
            core::math::Modulo(lhs.constant().number(),
                               rhs.constant().number()));
      }
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry Lshift(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) <<
              (core::DoubleToInt32(rhs.constant().number()) & 0x1F)));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry Rshift(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) >>
              (core::DoubleToInt32(rhs.constant().number()) & 0x1F)));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry RshiftLogical(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::UInt32(
              core::DoubleToUInt32(lhs.constant().number()) >>
              (core::DoubleToInt32(rhs.constant().number()) & 0x1F)));
    }
    if (lhs.IsInt32()) {
      return TypeEntry(Type::Int32());
    }
    return TypeEntry(Type::Number());
  }

  static TypeEntry LT(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Bool(
              JSVal::NumberCompare(lhs.constant().number(),
                                   rhs.constant().number()) == CMP_TRUE));
    }
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry LTE(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Bool(
              JSVal::NumberCompare(rhs.constant().number(),
                                   lhs.constant().number()) == CMP_FALSE));
    }
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry GT(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Bool(
              JSVal::NumberCompare(rhs.constant().number(),
                                   lhs.constant().number()) == CMP_TRUE));
    }
    return TypeEntry(Type::Boolean());
  }

  static TypeEntry GTE(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Bool(
              JSVal::NumberCompare(lhs.constant().number(),
                                   rhs.constant().number()) == CMP_FALSE));
    }
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
              core::DoubleToInt32(rhs.constant().number())));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry BitwiseOr(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) |
              core::DoubleToInt32(rhs.constant().number())));
    }
    return TypeEntry(Type::Int32());
  }

  static TypeEntry BitwiseXor(const TypeEntry& lhs, const TypeEntry& rhs) {
    if (lhs.IsConstant() && lhs.constant().IsNumber() &&
        rhs.IsConstant() && rhs.constant().IsNumber()) {
      return TypeEntry(
          JSVal::Int32(
              core::DoubleToInt32(lhs.constant().number()) ^
              core::DoubleToInt32(rhs.constant().number())));
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
    if (src.IsString()) {
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

  static TypeEntry TypeOf(Context* ctx, const TypeEntry& src) {
    if (src.IsConstant()) {
      return TypeEntry(src.constant().TypeOf(ctx));
    }

    if (src.IsString()) {
      return TypeEntry(ctx->global_data()->string_string());
    } else if (src.IsNumber()) {
      return TypeEntry(ctx->global_data()->string_number());
    } else if (src.IsNull()) {
      return TypeEntry(ctx->global_data()->string_object());
    } else if (src.IsUndefined()) {
      return TypeEntry(ctx->global_data()->string_undefined());
    } else if (src.IsBoolean()) {
      return TypeEntry(ctx->global_data()->string_boolean());
    } else if (src.IsFunction()) {
      return TypeEntry(ctx->global_data()->string_function());
    } else if (src.IsArray()) {
      return TypeEntry(ctx->global_data()->string_object());
    }

    return TypeEntry(Type::String());
  }

 private:
  JSVal constant_;
};

class TypeRecord {
 public:
  typedef std::unordered_map<register_t, TypeEntry> Record;

  static bool IsConstantID(register_t offset) {
    return (offset - railgun::FrameConstant<>::kFrameSize) >=
        railgun::FrameConstant<>::kConstantOffset;
  }

  static bool IsThis(register_t offset) {
    return (offset - railgun::FrameConstant<>::kFrameSize) ==
        railgun::FrameConstant<>::kThisOffset;
  }

  static uint32_t ExtractConstantOffset(register_t reg) {
    assert(IsConstantID(reg));
    return reg -
        railgun::FrameConstant<>::kFrameSize -
        railgun::FrameConstant<>::kConstantOffset;
  }

  TypeRecord()
    : record_(),
      code_(nullptr) {
  }

  void Init(railgun::Code* code) {
    record_.clear();
    code_ = code;
  }

  void Clear() {
    record_.clear();
  }

  TypeEntry Get(register_t offset) {
    if (IsConstantID(offset)) {
      return TypeEntry(code_->constants()[ExtractConstantOffset(offset)]);
    }
    if (!code_->strict() && IsThis(offset)) {
      return TypeEntry(Type::Object());
    }
    return record_[offset];
  }

  void Put(register_t offset, const TypeEntry& entry) {
    record_[offset] = entry;
  }

 private:
  Record record_;
  railgun::Code* code_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TYPE_H_
