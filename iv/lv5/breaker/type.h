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

  bool IsNumber() const { return (type_ & TYPE_NUMBER_MASK); }

  bool IsString() const { return type_ == TYPE_STRING; }

  bool IsBoolean() const { return type_ == TYPE_BOOLEAN; }

  bool IsUndefined() const { return type_ == TYPE_UNDEFINED; }

  bool IsNull() const { return type_ == TYPE_NULL; }
 private:
  TypeValue type_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TYPE_H_
