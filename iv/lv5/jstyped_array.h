#ifndef IV_LV5_JSTYPED_ARRAY_H_
#define IV_LV5_JSTYPED_ARRAY_H_
#include <iv/detail/cstdint.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsarray_buffer.h>
#include <iv/lv5/binary_blocks.h>
namespace iv {
namespace lv5 {

template<typename T, typename Derived>
class TypedArrayImpl : public JSObject {
 public:
  typedef T Element;

  enum FIELD {
    FIELD_LENGTH = 0,
    FIELD_BYTE_LENGTH,
    FIELD_BUFFER,
    FIELD_BYTE_OFFSET
  };

  uint32_t length() const { return Direct(FIELD_LENGTH).int32(); }
  uint32_t byte_length() const { return Direct(FIELD_BYTE_LENGTH).int32(); }
  uint32_t byte_offset() const { return Direct(FIELD_BYTE_OFFSET).int32(); }
  JSArrayBuffer* buffer() const {
    return static_cast<JSArrayBuffer*>(Direct(FIELD_BUFFER).object());
  }

  static Derived* New(Context* ctx,
                      JSArrayBuffer* buf,
                      uint32_t byte_offset, uint32_t byte_length, uint32_t length) {
    Derived* obj =
        new Derived(ctx, buf,
                    byte_offset, byte_length, length,
                    ctx->global_data()->typed_array_map(TypedArrayTraits<T>::code));
    obj->set_cls(Derived::GetClass());
    return obj;
  }

  IV_LV5_INTERNAL_METHOD bool GetOwnIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot) {
    const TypedArrayImpl* impl = static_cast<const TypedArrayImpl*>(obj);
    JSArrayBuffer* buf = impl->buffer();
    const uint32_t off = impl->byte_offset();
    const uint64_t total = off + sizeof(T) * (index + 1);
    if (total > buf->length()) {
      return JSObject::GetOwnIndexedPropertySlotMethod(obj, ctx, index, slot);
    }
    const T result =
        buf->GetValue<T>(off, index, core::kLittleEndian);
    slot->set(result, Attributes::CreateData(ATTR::W | ATTR::E), obj);
    return true;
  }

  IV_LV5_INTERNAL_METHOD bool DefineOwnIndexedPropertySlotMethod(JSObject* obj,
                                                                 Context* ctx,
                                                                 uint32_t index,
                                                                 const PropertyDescriptor& desc,
                                                                 Slot* slot,
                                                                 bool throwable, Error* e) {
    const TypedArrayImpl* impl = static_cast<const TypedArrayImpl*>(obj);
    JSArrayBuffer* buf = impl->buffer();
    const uint32_t off = impl->byte_offset();
    const uint64_t total = off + sizeof(T) * (index + 1);
    if (total > buf->length()) {
      return JSObject::DefineOwnIndexedPropertySlotMethod(obj, ctx, index, desc, slot, throwable, e);
    }

    // FIXME(Constellation) writable check is not found...
    JSVal value;
    if (desc.IsData() && !desc.IsValueAbsent()) {
      value = desc.AsDataDescriptor()->value();
    }
    const T converted = TypedArrayTraits<T>::ToType(ctx, value, IV_LV5_ERROR(e));
    buf->SetValue<T>(off, index, core::kLittleEndian, converted);
    return true;
  }

  IV_LV5_INTERNAL_METHOD void GetOwnPropertyNamesMethod(const JSObject* obj,
                                                        Context* ctx,
                                                        PropertyNamesCollector* collector,
                                                        EnumerationMode mode) {
    const TypedArrayImpl* impl = static_cast<const TypedArrayImpl*>(obj);
    for (uint32_t i = 0, iz = impl->length(); i < iz; ++i) {
      collector->Add(i);
    }
    JSObject::GetOwnPropertyNamesMethod(obj, ctx, collector, mode);
  }

 public:
  TypedArrayImpl(Context* ctx,
                 JSArrayBuffer* buf,
                 uint32_t byte_offset,
                 uint32_t byte_length, uint32_t length, Map* map)
    : JSObject(map) {
    Direct(FIELD_LENGTH) = JSVal::UInt32(length);
    Direct(FIELD_BYTE_LENGTH) = JSVal::UInt32(byte_length);
    Direct(FIELD_BYTE_OFFSET) = JSVal::UInt32(byte_offset);
    Direct(FIELD_BUFFER) = buf;
  }
};

#define IV_LV5_DEFINE_TYPED_ARRAY(TYPE, NAME)\
    class JS##NAME : public TypedArrayImpl<TYPE, JS##NAME> {\
     public:\
      IV_LV5_DEFINE_JSCLASS(JS##NAME, NAME)\
      typedef TypedArrayImpl<TYPE, JS##NAME> impl_type;\
      static const Class::JSClassType ClassCode = Class::NAME;\
      JS##NAME(Context* ctx,\
               JSArrayBuffer* buf,\
               uint32_t byte_offset,\
               uint32_t byte_length, uint32_t length, Map* map)\
        : impl_type(ctx, buf, byte_offset, byte_length, length, map) { }\
    }

IV_LV5_DEFINE_TYPED_ARRAY(int8_t, Int8Array);
IV_LV5_DEFINE_TYPED_ARRAY(uint8_t, Uint8Array);
IV_LV5_DEFINE_TYPED_ARRAY(int16_t, Int16Array);
IV_LV5_DEFINE_TYPED_ARRAY(uint16_t, Uint16Array);
IV_LV5_DEFINE_TYPED_ARRAY(int32_t, Int32Array);
IV_LV5_DEFINE_TYPED_ARRAY(uint32_t, Uint32Array);
IV_LV5_DEFINE_TYPED_ARRAY(float, Float32Array);
IV_LV5_DEFINE_TYPED_ARRAY(double, Float64Array);
IV_LV5_DEFINE_TYPED_ARRAY(Uint8Clamped, Uint8ClampedArray);

#undef IV_LV5_DEFINE_TYPED_ARRAY

} }  // namespace iv::lv5
#endif  // IV_LV5_JSDATA_VIEW_H_
