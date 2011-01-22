#ifndef _IV_LV5_RUNTIME_REGEXP_H_
#define _IV_LV5_RUNTIME_REGEXP_H_
#include "arguments.h"
#include "jsval.h"
#include "context.h"
#include "error.h"
#include "lv5.h"
#include "jsregexp.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

}  // namespace iv::lv5::runtime::detail

inline JSVal RegExpConstructor(const Arguments& args, Error* e) {
  const uint32_t args_count = args.size();
  Context* const ctx = args.ctx();
  JSString* pattern;
  if (args_count == 0) {
    return JSRegExp::New(ctx);
  } else {
    const JSVal& first = args[0];
    if (first.IsObject() &&
        ctx->Intern("RegExp") == first.object()->class_name()) {
      if (args_count > 1 && !args[1].IsUndefined()) {
        e->Report(Error::Type,
                  "RegExp Constructor with RegExp object and unknown flags");
        return JSUndefined;
      }
      return JSRegExp::New(ctx, static_cast<JSRegExp*>(first.object()));
    }
    pattern = args[0].ToString(ctx, ERROR(e));
  }
  if (args_count == 1 || args[1].IsUndefined()) {
    return JSRegExp::New(ctx, pattern->piece(), core::UStringPiece());
  }
  JSString* flags = args[1].ToString(ctx, ERROR(e));
  return JSRegExp::New(ctx, pattern->piece(), flags->piece());
}

// section 15.10.6.4 RegExp.prototype.toString()
inline JSVal RegExpToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("RegExp.prototype.toString", args, error);
  Context* const ctx = args.ctx();
  const JSVal& obj = args.this_binding();
  if (obj.IsObject() &&
      ctx->Intern("RegExp") == obj.object()->class_name()) {
    JSRegExp* const reg = static_cast<JSRegExp*>(obj.object());
    JSStringBuilder builder(ctx);
    builder.Append('/');
    builder.Append(*(reg->source(ctx)));
    builder.Append('/');
    if (reg->global()) {
      builder.Append('g');
    }
    if (reg->ignore()) {
      builder.Append('i');
    }
    if (reg->multiline()) {
      builder.Append('m');
    }
    return builder.Build();
  }
  error->Report(Error::Type,
                "RegExp.prototype.toString is not generic function");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_REGEXP_H_
