#ifndef _IV_LV5_JSEXCEPTION_H_
#define _IV_LV5_JSEXCEPTION_H_
#include "jsobject.h"
#include "error.h"
namespace iv {
namespace lv5 {

class JSVal;

class JSError : public JSObject {
 public:
  static JSVal CreateFromCode(Error::Code code);
  explicit JSError(Error::Code code);
 protected:
  Error::Code code_;
};

class JSReferenceError : public JSError {
};

class JSTypeError : public JSError {
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSEXCEPTION_H_
