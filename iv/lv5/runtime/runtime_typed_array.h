#ifndef IV_LV5_RUNTIME_TYPED_ARRAY_H_
#define IV_LV5_RUNTIME_TYPED_ARRAY_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jstyped_array.h>
namespace iv {
namespace lv5 {
namespace runtime {

template<typename Type, typename ArrayType>
inline JSVal TypedArrayConstructor(const Arguments& args, Error* e) {
  Context* const ctx = args.ctx();
  // JSObject* const obj = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  const JSVal first = args.At(0);
  if (first.IsNumber()) {
    const uint32_t len = first.ToUInt32(ctx, IV_LV5_ERROR(e));
    const uint32_t byte_length = len * sizeof(Type);
    JSArrayBuffer* buffer = JSArrayBuffer::New(ctx, byte_length, IV_LV5_ERROR(e));
    return ArrayType::New(ctx, buffer, 0, byte_length, len);
  }
  JSObject* src = first.ToObject(ctx, IV_LV5_ERROR(e));
  if (src->IsClass<Class::ArrayBuffer>()) {
    JSArrayBuffer* buffer = static_cast<JSArrayBuffer*>(src);
    const uint32_t byte_offset = args.At(1).ToUInt32(ctx, IV_LV5_ERROR(e));
    if (byte_offset % sizeof(Type) != 0) {
      e->Report(Error::Range, "byte offset value is not multiple of the element size in bytes");
      return JSEmpty;
    }
    uint32_t buffer_length = buffer->length();
    uint32_t byte_length = buffer_length - byte_offset;
    if (args.size() >= 3) {
      byte_length = args.At(2).ToUInt32(ctx, IV_LV5_ERROR(e));
    }
    if (byte_offset + byte_length > buffer_length) {
      e->Report(Error::Range, "offset + length value is out of range");
      return JSEmpty;
    }
    if (byte_length % sizeof(Type) != 0) {
      e->Report(Error::Range, "byte length value is not multiple of the element size in bytes");
      return JSEmpty;
    }
    const uint32_t length = byte_length / sizeof(Type);
    return ArrayType::New(ctx, buffer, byte_offset, byte_length, length);
  }

  const uint32_t length = internal::GetLength(ctx, src, IV_LV5_ERROR(e));
  const uint32_t byte_length = length * sizeof(Type);
  JSArrayBuffer* buffer = JSArrayBuffer::New(ctx, byte_length, IV_LV5_ERROR(e));
  for (uint32_t i = 0; i < length; ++i) {
    const JSVal value =
        src->Get(ctx, symbol::MakeSymbolFromIndex(i), IV_LV5_ERROR(e));
    const Type converted =
        TypedArrayTraits<Type>::ToType(ctx, value, IV_LV5_ERROR(e));
    buffer->SetValue<Type>(0, i, core::kLittleEndian, converted);
  }
  return ArrayType::New(ctx, buffer, 0, byte_length, length);
}

template<typename Type, typename ArrayType>
inline JSVal TypedArraySet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("TypedArray.prototype.set", args, e);
  Context* const ctx = args.ctx();
  JSObject* t = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!t->IsClass<ArrayType::ClassCode>()) {
    e->Report(Error::Type,
              "TypedArray.prototype.set is not generic function");
    return JSEmpty;
  }
  ArrayType* typed = static_cast<ArrayType*>(t);
  uint32_t offset = args.At(1).ToUInt32(ctx, IV_LV5_ERROR(e));
  JSObject* src = args.At(0).ToObject(ctx, IV_LV5_ERROR(e));
  const uint32_t src_length = internal::GetLength(ctx, src, IV_LV5_ERROR(e));
  const uint32_t target_length = typed->length();
  if (src_length + offset > target_length) {
    e->Report(Error::Range,
              "source length + offset is greater than target length");
    return JSEmpty;
  }
  JSArrayBuffer* buffer = typed->buffer();
  for (uint32_t i = offset; i < target_length; ++i) {
    const JSVal value =
        src->Get(ctx, symbol::MakeSymbolFromIndex(i - offset), IV_LV5_ERROR(e));
    const Type converted =
        TypedArrayTraits<Type>::ToType(ctx, value, IV_LV5_ERROR(e));
    buffer->SetValue<Type>(0, i, core::kLittleEndian, converted);
  }
  return JSUndefined;
}

template<typename Type, typename ArrayType>
inline JSVal TypedArraySubarray(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("TypedArray.prototype.subarray", args, e);
  Context* const ctx = args.ctx();
  JSObject* t = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!t->IsClass<ArrayType::ClassCode>()) {
    e->Report(Error::Type,
              "TypedArray.prototype.subarray is not generic function");
    return JSEmpty;
  }
  ArrayType* typed = static_cast<ArrayType*>(t);
  const uint32_t src_length = typed->length();
  int32_t begin_int = args.At(0).ToInt32(ctx, IV_LV5_ERROR(e));
  if (begin_int < 0) {
    begin_int = src_length + begin_int;
    if (begin_int < 0) {
      begin_int = 0;
    }
  }
  const uint32_t begin_index = std::min<uint32_t>(src_length, static_cast<uint32_t>(begin_int));
  int32_t end_int = src_length;
  if (args.size() >= 2) {
    end_int = args[1].ToInt32(ctx, IV_LV5_ERROR(e));
    if (end_int < 0) {
      end_int = src_length + end_int;
      if (end_int < 0) {
        end_int = 0;
      }
    }
  }
  uint32_t end_index = std::min<uint32_t>(src_length, static_cast<uint32_t>(end_int));
  if (end_index < begin_index) {
    end_index = begin_index;
  }
  const uint32_t length = end_index - begin_index;
  return ArrayType::New(
      ctx,
      typed->buffer(),
      typed->byte_offset() + sizeof(Type) * begin_index,
      length * sizeof(Type), length);
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_TYPED_ARRAY_H_
