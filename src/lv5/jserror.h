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
  static JSVal Detail(Context* ctx, const Error* error);
  static JSError* New(Context* ctx, Error::Code code, JSString* str);
  static JSError* NewNativeError(Context* ctx, JSString* str);
  static JSError* NewEvalError(Context* ctx, JSString* str);
  static JSError* NewRangeError(Context* ctx, JSString* str);
  static JSError* NewReferenceError(Context* ctx, JSString* str);
  static JSError* NewSyntaxError(Context* ctx, JSString* str);
  static JSError* NewTypeError(Context* ctx, JSString* str);
  static JSError* NewURIError(Context* ctx, JSString* str);
 protected:
  Error::Code code_;
  JSString* detail_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSEXCEPTION_H_
