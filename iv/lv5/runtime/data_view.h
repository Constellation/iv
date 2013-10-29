#ifndef IV_LV5_RUNTIME_DATA_VIEW_H_
#define IV_LV5_RUNTIME_DATA_VIEW_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.13.7.1 DataView(buffer [, byteOffset [, byteLength]])
// section 15.13.7.2.1 new DataView(buffer [, byteOffset [, byteLength]])
JSVal DataViewConstructor(const Arguments& args, Error* e);

// section 15.13.7.4.2 DataView.prototype.getInt8(byteOffset)
JSVal DataViewGetInt8(const Arguments& args, Error* e);

// section 15.13.7.4.3 DataView.prototype.getUint8(byteOffset)
JSVal DataViewGetUint8(const Arguments& args, Error* e);

// section 15.13.7.4.4 DataView.prototype.getInt16(byteOffset, isLittleEndian)
JSVal DataViewGetInt16(const Arguments& args, Error* e);

// section 15.13.7.4.5 DataView.prototype.getUint16(byteOffset, isLittleEndian)
JSVal DataViewGetUint16(const Arguments& args, Error* e);

// section 15.13.7.4.6 DataView.prototype.getInt32(byteOffset, isLittleEndian)
JSVal DataViewGetInt32(const Arguments& args, Error* e);

// section 15.13.7.4.7 DataView.prototype.getUint32(byteOffset, isLittleEndian)
JSVal DataViewGetUint32(const Arguments& args, Error* e);

// section 15.13.7.4.8 DataView.prototype.getFloat32(byteOffset, isLittleEndian)
JSVal DataViewGetFloat32(const Arguments& args, Error* e);

// section 15.13.7.4.9 DataView.prototype.getFloat64(byteOffset, isLittleEndian)
JSVal DataViewGetFloat64(const Arguments& args, Error* e);

// section 15.13.7.4.10 DataView.prototype.setInt8(byteOffset, value)
JSVal DataViewSetInt8(const Arguments& args, Error* e);

// section 15.13.7.4.11 DataView.prototype.setUint8(byteOffset, value)
JSVal DataViewSetUint8(const Arguments& args, Error* e);

// section 15.13.7.4.12 DataView.prototype.setInt16(byteOffset, value, isLittleEndian)
JSVal DataViewSetInt16(const Arguments& args, Error* e);

// section 15.13.7.4.13 DataView.prototype.setUint16(byteOffset, value, isLittleEndian)
JSVal DataViewSetUint16(const Arguments& args, Error* e);

// section 15.13.7.4.14 DataView.prototype.setInt32(byteOffset, value, isLittleEndian)
JSVal DataViewSetInt32(const Arguments& args, Error* e);

// section 15.13.7.4.15 DataView.prototype.setUint32(byteOffset, value, isLittleEndian)
JSVal DataViewSetUint32(const Arguments& args, Error* e);

// section 15.13.7.4.16 DataView.prototype.setFloat32(byteOffset, value, isLittleEndian)
JSVal DataViewSetFloat32(const Arguments& args, Error* e);

// section 15.13.7.4.17 DataView.prototype.setFloat64(byteOffset, value, isLittleEndian)
JSVal DataViewSetFloat64(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_DATA_VIEW_H_
