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

inline JSVal RegExpConstructor(const Arguments& args, Error* error) {
  return JSRegExp::New(args.ctx());
}

// section 15.10.6.4 RegExp.prototype.toString()
inline JSVal RegExpToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("RegExp.prototype.toString", args, error);
  Context* const ctx = args.ctx();
  const JSVal& obj = args.this_binding();
  if (obj.IsObject() &&
      args.ctx()->Intern("RegExp") == obj.object()->class_name()) {
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
