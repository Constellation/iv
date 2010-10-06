#ifndef _IV_LV5_JSEXCEPTION_H_
#define _IV_LV5_JSEXCEPTION_H_
#include "jserrorcode.h"
#include "jsobject.h"
namespace iv {
namespace lv5 {

class JSVal;

class JSError : public JSObject {
 public:
  static JSVal CreateFromCode(JSErrorCode::Type code);
  explicit JSError(JSErrorCode::Type code);
 protected:
  JSErrorCode::Type code_;
};

class JSReferenceError : public JSError {
};

class JSTypeError : public JSError {
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSEXCEPTION_H_
