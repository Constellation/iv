#include "jserror.h"
#include "jsval.h"
#include "context.h"

namespace iv {
namespace lv5 {

JSError::JSError(Context* ctx, Error::Code code, JSString* str)
  : code_(code) {
  DefineOwnProperty(ctx, ctx->Intern("message"),
                         DataDescriptor(str,
                                        PropertyDescriptor::WRITABLE),
                                        false, NULL);
}

JSError* JSError::New(Context* ctx, Error::Code code, JSString* str) {
  JSError* const error = new JSError(ctx, code, str);
  const Class& cls = ctx->Cls("Error");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewNativeError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::Native, str);
  const Class& cls = ctx->Cls("NativeError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewEvalError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::Eval, str);
  const Class& cls = ctx->Cls("EvalError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewRangeError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::Range, str);
  const Class& cls = ctx->Cls("RangeError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewReferenceError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::Reference, str);
  const Class& cls = ctx->Cls("ReferenceError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewSyntaxError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::Syntax, str);
  const Class& cls = ctx->Cls("SyntaxError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewTypeError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::Type, str);
  const Class& cls = ctx->Cls("TypeError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSError* JSError::NewURIError(Context* ctx, JSString* str) {
  JSError* const error = new JSError(ctx, Error::URI, str);
  const Class& cls = ctx->Cls("URIError");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

} }  // namespace iv::lv5
