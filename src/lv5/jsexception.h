#ifndef _IV_LV5_JSEXCEPTION_H_
#define _IV_LV5_JSEXCEPTION_H_
#include "jsobject.h"
#include "error.h"
namespace iv {
namespace lv5 {

class JSVal;
class JSString;

class JSError : public JSObject {
 public:
  explicit JSError(Context* ctx, Error::Code code, JSString* str);
  static JSError* New(Context* ctx, Error::Code code, JSString* str);
 protected:
  Error::Code code_;
  JSString* detail_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSEXCEPTION_H_
