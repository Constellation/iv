#ifndef IV_LV5_JSDATA_VIEW_H_
#define IV_LV5_JSDATA_VIEW_H_
#include <iv/detail/cstdint.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray_buffer.h>
namespace iv {
namespace lv5 {

class JSDataView : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(DataView)
  enum FIELD {
    FIELD_BYTE_LENGTH = 0,
    FIELD_BUFFER = 1,
    FIELD_BYTE_OFFSET = 2
  };

  uint32_t length() const { return Direct(FIELD_BYTE_LENGTH).int32(); }
  uint32_t offset() const { return Direct(FIELD_BYTE_OFFSET).int32(); }
  JSArrayBuffer* buffer() const {
    return static_cast<JSArrayBuffer*>(Direct(FIELD_BUFFER).object());
  }

  static JSDataView* New(Context* ctx,
                         JSArrayBuffer* buf,
                         uint32_t offset, uint32_t len) {
    JSDataView* obj =
        NewPlain(ctx, buf, offset, len, ctx->global_data()->data_view_map());
    obj->set_cls(GetClass());
    obj->set_prototype(ctx->global_data()->data_view_prototype());
    return obj;
  }

  static JSDataView* NewPlain(Context* ctx,
                              JSArrayBuffer* buf,
                              uint32_t offset, uint32_t len, Map* map) {
    assert(buf);
    return new JSDataView(ctx, buf, offset, len, map);
  }

  template<typename Type>
  Type GetValue(uint32_t off, bool is_little_endian, Error* e) const {
    JSArrayBuffer* buf = buffer();
    const uint64_t total = off + offset() + sizeof(Type);
    if (total > buf->length()) {
      e->Report(Error::Range, "offset out of range");
      return Type();
    }
    return buf->GetValue<Type>(
        static_cast<uint32_t>(total), 0, is_little_endian);
  }

  template<typename Type>
  void SetValue(uint32_t off, bool is_little_endian, Type value, Error* e) {
    JSArrayBuffer* buf = buffer();
    const uint64_t total = off + offset() + sizeof(Type);
    if (total > buf->length()) {
      e->Report(Error::Range, "offset out of range");
      return;
    }
    buf->SetValue<Type>(
        static_cast<uint32_t>(total), 0, is_little_endian, value);
  }

 private:
  JSDataView(Context* ctx,
             JSArrayBuffer* buf,
             uint32_t offset, uint32_t len, Map* map)
    : JSObject(map) {
    Direct(FIELD_BYTE_OFFSET) = JSVal::UInt32(offset);
    Direct(FIELD_BYTE_LENGTH) = JSVal::UInt32(len);
    Direct(FIELD_BUFFER) = buf;
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSDATA_VIEW_H_
