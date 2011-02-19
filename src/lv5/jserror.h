#ifndef _IV_LV5_JSEXCEPTION_H_
#define _IV_LV5_JSEXCEPTION_H_
#include <cassert>
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/jsval.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {

class JSVal;
class JSString;

class JSError : public JSObject {
 public:
  JSError(Context* ctx, Error::Code code, JSString* str)
    : code_(code) {
    if (str) {
      DefineOwnProperty(ctx, context::Intern(ctx, "message"),
                             DataDescriptor(str,
                                            PropertyDescriptor::CONFIGURABLE |
                                            PropertyDescriptor::ENUMERABLE |
                                            PropertyDescriptor::WRITABLE),
                                            false, NULL);
    }
  }


  static JSVal Detail(Context* ctx, const Error* error) {
    assert(error && (error->code() != Error::Normal));
    switch (error->code()) {
      case Error::Native:
        return JSError::NewNativeError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::Eval:
        return JSError::NewEvalError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::Range:
        return JSError::NewRangeError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::Reference:
        return JSError::NewReferenceError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::Syntax:
        return JSError::NewSyntaxError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::Type:
        return JSError::NewTypeError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::URI:
        return JSError::NewURIError(
            ctx, JSString::New(ctx, error->detail()));
      case Error::User:
        return error->value();
      default:
        UNREACHABLE();
        return JSUndefined;  // make compiler happy
    };
  }


  static JSError* New(Context* ctx, Error::Code code, JSString* str) {
    JSError* const error = new JSError(ctx, code, str);
    const Class& cls = context::Cls(ctx, "Error");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewNativeError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Native, str);
    const Class& cls = context::Cls(ctx, "NativeError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewEvalError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Eval, str);
    const Class& cls = context::Cls(ctx, "EvalError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewRangeError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Range, str);
    const Class& cls = context::Cls(ctx, "RangeError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewReferenceError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Reference, str);
    const Class& cls = context::Cls(ctx, "ReferenceError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewSyntaxError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Syntax, str);
    const Class& cls = context::Cls(ctx, "SyntaxError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewTypeError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Type, str);
    const Class& cls = context::Cls(ctx, "TypeError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


  static JSError* NewURIError(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::URI, str);
    const Class& cls = context::Cls(ctx, "URIError");
    error->set_class_name(cls.name);
    error->set_prototype(cls.prototype);
    return error;
  }


 protected:
  Error::Code code_;
  JSString* detail_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSEXCEPTION_H_
