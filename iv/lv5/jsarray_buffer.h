#ifndef IV_LV5_JSARRAY_BUFFER_H_
#define IV_LV5_JSARRAY_BUFFER_H_
#include <iv/byteorder.h>
#include <iv/detail/cstdint.h>
#include <iv/lv5/jsobject_fwd.h>
namespace iv {
namespace lv5 {

template<typename T>
inline T GetFromBuffer(
    const uint8_t* bytes, bool is_little_endian) {
  union {
    uint8_t bytes[sizeof(T)];
    T raw;
  } data;
  if (is_little_endian == !!core::kLittleEndian) {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      data.bytes[i] = bytes[i];
    }
  } else {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      data.bytes[sizeof(T) - i - 1] = bytes[i];
    }
  }
  return data.raw;
}

template<>
inline Uint8Clamped GetFromBuffer<Uint8Clamped>(const uint8_t* bytes, bool is_little_endian) {
  return Uint8Clamped::UInt8(*bytes);
}

template<typename T>
inline void SetToBuffer(uint8_t* bytes, bool is_little_endian, T value) {
  union {
    uint8_t bytes[sizeof(T)];
    T raw;
  } data;
  data.raw = value;
  if (is_little_endian == !!core::kLittleEndian) {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      bytes[i] = data.bytes[i];
    }
  } else {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      bytes[sizeof(T) - i - 1] = data.bytes[i];
    }
  }
}

template<>
inline void SetToBuffer<Uint8Clamped>(uint8_t* bytes, bool is_little_endian, Uint8Clamped value) {
  *bytes = value;
}

class JSArrayBuffer : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSArrayBuffer, ArrayBuffer)
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
            ctx->global_data()->array_buffer_map(),
            IV_LV5_ERROR_WITH(e, NULL));
    obj->set_cls(GetClass());
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

  template<typename Type>
  Type GetValue(uint32_t offset, uint32_t index, bool is_little_endian) const {
    const uint32_t slide = offset + index * sizeof(Type);
    const uint8_t* const ptr = data_.u8 + slide;
    const uintptr_t ptr_value = reinterpret_cast<uintptr_t>(ptr);
    if (is_little_endian == !!core::kLittleEndian && ptr_value % sizeof(Type) == 0) {
      return *reinterpret_cast<const Type*>(ptr);
    }
    return GetFromBuffer<Type>(ptr, is_little_endian);
  }

  template<typename Type>
  void SetValue(uint32_t offset, uint32_t index, bool is_little_endian, Type value) {
    const uint32_t slide = offset + index * sizeof(Type);
    uint8_t* const ptr = data_.u8 + slide;
    const uintptr_t ptr_value = reinterpret_cast<uintptr_t>(ptr);
    if (is_little_endian == !!core::kLittleEndian && ptr_value % sizeof(Type) == 0) {
      *reinterpret_cast<Type*>(ptr) = value;
      return;
    }
    SetToBuffer<Type>(ptr, is_little_endian, value);
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
