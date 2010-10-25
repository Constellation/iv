#include "jsexception.h"
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

} }  // namespace iv::lv5
