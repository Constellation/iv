#include "jsregexp.h"
#include "context.h"
namespace iv {
namespace lv5 {

JSRegExp::JSRegExp(Context* ctx,
                   const core::UStringPiece& value,
                   const core::UStringPiece& flags)
  : impl_(new JSRegExpImpl(value, flags)) {
  DefineOwnProperty(ctx, ctx->Intern("source"),
                    DataDescriptor(JSString::New(ctx, value),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());
}

JSRegExp::JSRegExp(Context* ctx,
                   const core::UStringPiece& value,
                   const core::UStringPiece& flags,
                   const JSRegExpImpl* reg)
  : impl_(reg) {
  DefineOwnProperty(ctx, ctx->Intern("source"),
                    DataDescriptor(JSString::New(ctx, value),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());
}

JSRegExp::JSRegExp(Context* ctx)
  : impl_(new JSRegExpImpl()) {
  DefineOwnProperty(ctx, ctx->Intern("source"),
                    DataDescriptor(JSString::New(ctx, detail::kEmptyPattern),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());
}

JSRegExp* JSRegExp::New(Context* ctx) {
  JSRegExp* const reg = new JSRegExp(ctx);
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::New(Context* ctx,
                        const core::UStringPiece& value,
                        const core::UStringPiece& flags,
                        const JSRegExpImpl* impl) {
  JSRegExp* const reg = new JSRegExp(ctx, value, flags, impl);
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::New(Context* ctx,
                        const core::UStringPiece& value,
                        const core::UStringPiece& flags) {
  JSRegExp* const reg = new JSRegExp(ctx, value, flags);
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::NewPlain(Context* ctx) {
  return new JSRegExp(ctx);
}

} }  // namespace iv::lv5
