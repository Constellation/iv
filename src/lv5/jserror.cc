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
                                        false, ctx->error());
}

JSError* JSError::New(Context* ctx, Error::Code code, JSString* str) {
  JSError* const error = new JSError(ctx, code, str);
  const Class& cls = ctx->Cls("Error");
  error->set_cls(cls.name);
  error->set_prototype(cls.prototype);
  return error;
}

JSVal JSError::Detail(Context* ctx, const Error* error) {
  assert(error && (error->code() != Error::Normal));
  switch (error->code()) {
    case Error::Native:
      return JSError::NewNativeError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::Eval:
      return JSError::NewEvalError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::Range:
      return JSError::NewRangeError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::Reference:
      return JSError::NewReferenceError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::Syntax:
      return JSError::NewSyntaxError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::Type:
      return JSError::NewTypeError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::URI:
      return JSError::NewURIError(
          ctx, JSString::NewAsciiString(ctx, error->detail()));
    case Error::User:
      return error->value();
    default:
      UNREACHABLE();
      return JSUndefined;  // make compiler happy
  };
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
