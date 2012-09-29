#ifndef IV_LV5_JSARRAY_BUFFER_H_
#define IV_LV5_JSARRAY_BUFFER_H_
#include <iv/detail/cstdint.h>
#include <iv/lv5/jsobject.h>
namespace iv {
namespace lv5 {

class JSArrayBuffer : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(ArrayBuffer)
  enum FIELD {
    FIELD_BYTE_LENGTH = 0
  };

  union Data {
    int8_t* i8;
    uint8_t* u8;
    int16_t* i16;
    uint16_t* u16;
    uint32_t* u32;
    float* f32;
    double* f64;
  };

  JSArrayBuffer(Context* ctx, uint32_t len, Map* map)
    : JSObject(map) {
    Direct(FIELD_BYTE_LENGTH) = JSVal::UInt32(len);
    if (len) {
      data_.u8 = new (PointerFreeGC) uint8_t[len];
      std::memset(data_.u8, 0, len);
    } else {
      data_.u8 = NULL;
    }
  }

  static JSArrayBuffer* New(Context* ctx, uint32_t len) {
    JSArrayBuffer* obj =
        NewPlain(ctx, len, ctx->global_data()->GetArrayBufferMap());
    obj->set_cls(GetClass());
    obj->set_prototype(ctx->global_data()->array_buffer_prototype());
    return obj;
  }

  static JSArrayBuffer* NewPlain(Context* ctx, uint32_t len, Map* map) {
    return new JSArrayBuffer(ctx, len, map);
  }
 private:
 Data data_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSARRAY_BUFFER_H_
