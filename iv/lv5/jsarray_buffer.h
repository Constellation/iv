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

  static const uint32_t kMaxLength = INT32_MAX;

  uint32_t length() const {
    return Direct(FIELD_BYTE_LENGTH).int32();
  }

  bool static IsTooBigBuffer(uint32_t len) {
    return len > kMaxLength;
  }

  static JSArrayBuffer* New(Context* ctx, uint32_t len, Error* e) {
    JSArrayBuffer* obj =
        NewPlain(
            ctx, len,
            ctx->global_data()->GetArrayBufferMap(),
            IV_LV5_ERROR_WITH(e, NULL));
    obj->set_cls(GetClass());
    obj->set_prototype(ctx->global_data()->array_buffer_prototype());
    return obj;
  }

  static JSArrayBuffer* NewPlain(Context* ctx,
                                 uint32_t len, Map* map, Error* e) {
    if (IsTooBigBuffer(len)) {
      e->Report(Error::Range, "too big buffer");
      return NULL;
    }
    return new JSArrayBuffer(ctx, len, map, e);
  }

 private:
  JSArrayBuffer(Context* ctx, uint32_t len, Map* map, Error* e)
    : JSObject(map) {
    Direct(FIELD_BYTE_LENGTH) = JSVal::UInt32(len);
    if (len) {
      data_.u8 = new (PointerFreeGC) uint8_t[len];
      std::memset(data_.u8, 0, len);
    } else {
      data_.u8 = NULL;
    }
  }

 Data data_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSARRAY_BUFFER_H_
