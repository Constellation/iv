#ifndef IV_LV5_RUNTIME_DATA_VIEW_H_
#define IV_LV5_RUNTIME_DATA_VIEW_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsdata_view.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.13.7.1 DataView(buffer [, byteOffset [, byteLength]])
// section 15.13.7.2.1 new DataView(buffer [, byteOffset [, byteLength]])
inline JSVal DataViewConstructor(const Arguments& args, Error* e) {
  const JSVal first = args.At(0);
  if (!first.IsObject() || !first.object()->IsClass<Class::ArrayBuffer>()) {
    e->Report(Error::Type,
              "first argument of DataView construcotr should be ArrayBuffer");
    return JSEmpty;
  }
  JSArrayBuffer* buffer = static_cast<JSArrayBuffer*>(first.object());
  uint32_t offset = 0;
  if (args.size() >= 2) {
    offset = args[1].ToUInt32(args.ctx(), IV_LV5_ERROR(e));
  }
  const uint32_t original = buffer->length();
  if (original < offset) {
    e->Report(Error::Range, "too big offset");
    return JSEmpty;
  }
  uint32_t len = original - offset;
  if (args.size() >= 3) {
    len = args[2].ToUInt32(args.ctx(), IV_LV5_ERROR(e));
  }
  if (offset + len > original) {
    e->Report(Error::Range, "too big offset + length");
    return JSEmpty;
  }
  return JSDataView::New(args.ctx(), buffer, offset, len);
}

// section 15.13.7.4.2 DataView.prototype.getInt8(byteOffset)
inline JSVal DataViewGetInt8(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getInt8", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getInt8 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return JSVal::Signed(
      static_cast<JSDataView*>(view)->GetValue<int8_t>(offset, true, e));
}

// section 15.13.7.4.3 DataView.prototype.getUint8(byteOffset)
inline JSVal DataViewGetUint8(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getUint8", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getUint8 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return JSVal::UnSigned(
      static_cast<JSDataView*>(view)->GetValue<uint8_t>(offset, true, e));
}

// section 15.13.7.4.4 DataView.prototype.getInt16(byteOffset, isLittleEndian)
inline JSVal DataViewGetInt16(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getInt16", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getInt16 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return JSVal::Signed(
      static_cast<JSDataView*>(view)->GetValue<int16_t>(offset, args.At(1).ToBoolean(), e));
}

// section 15.13.7.4.5 DataView.prototype.getUint16(byteOffset, isLittleEndian)
inline JSVal DataViewGetUint16(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getUint16", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getUint16 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return JSVal::UnSigned(
      static_cast<JSDataView*>(view)->GetValue<uint16_t>(offset, args.At(1).ToBoolean(), e));
}

// section 15.13.7.4.6 DataView.prototype.getInt32(byteOffset, isLittleEndian)
inline JSVal DataViewGetInt32(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getInt32", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getInt32 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return JSVal::Signed(
      static_cast<JSDataView*>(view)->GetValue<int32_t>(offset, args.At(1).ToBoolean(), e));
}

// section 15.13.7.4.7 DataView.prototype.getUint32(byteOffset, isLittleEndian)
inline JSVal DataViewGetUint32(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getUint32", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getUint32 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return JSVal::UnSigned(
      static_cast<JSDataView*>(view)->GetValue<uint32_t>(offset, args.At(1).ToBoolean(), e));
}

// section 15.13.7.4.8 DataView.prototype.getFloat32(byteOffset, isLittleEndian)
inline JSVal DataViewGetFloat32(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getFloat32", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getFloat32 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return static_cast<double>(
      static_cast<JSDataView*>(view)->GetValue<float>(offset, args.At(1).ToBoolean(), e));
}

// section 15.13.7.4.9 DataView.prototype.getFloat64(byteOffset, isLittleEndian)
inline JSVal DataViewGetFloat64(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.getFloat64", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.getFloat64 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  return static_cast<JSDataView*>(view)->GetValue<double>(offset, args.At(1).ToBoolean(), e);
}

// section 15.13.7.4.10 DataView.prototype.setInt8(byteOffset, value)
inline JSVal DataViewSetInt8(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setInt8", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setInt8 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const int8_t value = args.At(1).ToInt32(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<int8_t>(offset, true, value, e);
  return JSUndefined;
}

// section 15.13.7.4.11 DataView.prototype.setUint8(byteOffset, value)
inline JSVal DataViewSetUint8(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setUint8", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setUint8 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const uint8_t value = args.At(1).ToUInt32(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<uint8_t>(offset, true, value, e);
  return JSUndefined;
}

// section 15.13.7.4.12 DataView.prototype.setInt16(byteOffset, value, isLittleEndian)
inline JSVal DataViewSetInt16(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setInt16", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setInt16 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const int16_t value = args.At(1).ToInt32(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<int16_t>(offset, args.At(2).ToBoolean(), value, e);
  return JSUndefined;
}

// section 15.13.7.4.13 DataView.prototype.setUint16(byteOffset, value, isLittleEndian)
inline JSVal DataViewSetUint16(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setUint16", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setUint16 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const uint16_t value = args.At(1).ToUInt32(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<uint16_t>(offset, args.At(2).ToBoolean(), value, e);
  return JSUndefined;
}

// section 15.13.7.4.14 DataView.prototype.setInt32(byteOffset, value, isLittleEndian)
inline JSVal DataViewSetInt32(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setInt32", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setInt32 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const int32_t value = args.At(1).ToInt32(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<int32_t>(offset, args.At(2).ToBoolean(), value, e);
  return JSUndefined;
}

// section 15.13.7.4.15 DataView.prototype.setUint32(byteOffset, value, isLittleEndian)
inline JSVal DataViewSetUint32(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setUint32", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setUint32 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const uint32_t value = args.At(1).ToUInt32(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<uint32_t>(offset, args.At(2).ToBoolean(), value, e);
  return JSUndefined;
}

// section 15.13.7.4.16 DataView.prototype.setFloat32(byteOffset, value, isLittleEndian)
inline JSVal DataViewSetFloat32(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setFloat32", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setFloat32 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const float value = args.At(1).ToNumber(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<float>(offset, args.At(2).ToBoolean(), value, e);
  return JSUndefined;
}

// section 15.13.7.4.17 DataView.prototype.setFloat64(byteOffset, value, isLittleEndian)
inline JSVal DataViewSetFloat64(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("DataView.prototype.setFloat64", args, e);
  Context* const ctx = args.ctx();
  JSObject* view = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!view->IsClass<Class::DataView>()) {
    e->Report(Error::Type,
              "DataView.prototype.setFloat64 is not generic function");
    return JSEmpty;
  }
  const uint32_t offset = args.At(0).ToUInt32(ctx, IV_LV5_ERROR(e));
  const double value = args.At(1).ToNumber(ctx, IV_LV5_ERROR(e));
  static_cast<JSDataView*>(view)->SetValue<double>(offset, args.At(2).ToBoolean(), value, e);
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_DATA_VIEW_H_
