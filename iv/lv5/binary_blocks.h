#ifndef IV_LV5_BINARY_BLOCKS_H_
#define IV_LV5_BINARY_BLOCKS_H_
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {

class TypedCode {
 public:
  enum Code {
    Int8,
    Uint8,
    Int16,
    Uint16,
    Int32,
    Uint32,
    Float32,
    Float64
  };
  static const int kNumOfCodes = 8;
};

template<typename T>
class TypedArrayTraits;

#define IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(TYPE, NAME, FUNC)\
    class JS##NAME##Array;\
    template<>\
    class TypedArrayTraits<TYPE> {\
     public:\
      typedef JS##NAME##Array Type;\
      static const TypedCode::Code code = TypedCode::NAME;\
      static inline TYPE ToType(Context* ctx, JSVal value, Error* e) {\
        return value.FUNC(ctx, e);\
      }\
    }

IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(int8_t, Int8, ToInt32);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(uint8_t, Uint8, ToUInt32);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(int16_t, Int16, ToInt32);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(uint16_t, Uint16, ToUInt32);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(int32_t, Int32, ToInt32);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(uint32_t, Uint32, ToUInt32);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(float, Float32, ToNumber);
IV_LV5_DEFINE_TYPED_ARRAY_TRAITS(double, Float64, ToNumber);

#undef IV_LV5_DEFINE_TYPED_ARRAY_TRAITS

} }  // namespace iv::lv5
#endif  // IV_LV5_BINARY_BLOCKS_H_
