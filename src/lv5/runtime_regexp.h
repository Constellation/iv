#ifndef _IV_LV5_RUNTIME_REGEXP_H_
#define _IV_LV5_RUNTIME_REGEXP_H_
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/context.h"
#include "lv5/error.h"
#include "lv5/jsregexp.h"

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
      if (args.IsConstructorCalled()) {
        return JSRegExp::New(ctx, static_cast<JSRegExp*>(first.object()));
      } else {
        return first;
      }
    }
    if (first.IsUndefined()) {
      pattern = JSString::NewEmptyString(ctx);
    } else {
      pattern = args[0].ToString(ctx, ERROR(e));
    }
  }
  JSRegExp* reg;
  if (args_count == 1 || args[1].IsUndefined()) {
    reg = JSRegExp::New(ctx, pattern->piece(), core::UStringPiece());
  } else {
    JSString* flags = args[1].ToString(ctx, ERROR(e));
    reg = JSRegExp::New(ctx, pattern->piece(), flags->piece());
  }
  if (reg->IsValid()) {
    return reg;
  } else {
    e->Report(Error::Syntax,
              "RegExp Constructor with invalid pattern");
    return JSUndefined;
  }
}

// section 15.10.6.2 RegExp.prototype.exec(string)
inline JSVal RegExpExec(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("RegExp.prototype.exec", args, e);
  Context* const ctx = args.ctx();
  const JSVal& obj = args.this_binding();
  if (obj.IsObject() &&
      ctx->Intern("RegExp") == obj.object()->class_name()) {
    JSString* string;
    if (args.size() == 0) {
      string = JSString::NewAsciiString(ctx, "undefined");
    } else {
      string = args[0].ToString(ctx, ERROR(e));
    }
    JSRegExp* const reg = static_cast<JSRegExp*>(obj.object());
    return reg->Exec(ctx, string, e);
  }
  e->Report(Error::Type,
            "RegExp.prototype.exec is not generic function");
  return JSUndefined;
}

// section 15.10.6.3 RegExp.prototype.test(string)
inline JSVal RegExpTest(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("RegExp.prototype.test", args, e);
  Context* const ctx = args.ctx();
  const JSVal& obj = args.this_binding();
  if (obj.IsObject() &&
      ctx->Intern("RegExp") == obj.object()->class_name()) {
    JSString* string;
    if (args.size() == 0) {
      string = JSString::NewAsciiString(ctx, "undefined");
    } else {
      string = args[0].ToString(ctx, ERROR(e));
    }
    JSRegExp* const reg = static_cast<JSRegExp*>(obj.object());
    const JSVal result = reg->Exec(ctx, string, ERROR(e));
    return JSVal::Bool(!result.IsNull());
  }
  e->Report(Error::Type,
            "RegExp.prototype.test is not generic function");
  return JSUndefined;
}

// section 15.10.6.4 RegExp.prototype.toString()
inline JSVal RegExpToString(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("RegExp.prototype.toString", args, e);
  Context* const ctx = args.ctx();
  const JSVal& obj = args.this_binding();
  if (obj.IsObject() &&
      ctx->Intern("RegExp") == obj.object()->class_name()) {
    JSRegExp* const reg = static_cast<JSRegExp*>(obj.object());
    StringBuilder builder;
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
    return builder.Build(ctx);
  }
  e->Report(Error::Type,
            "RegExp.prototype.toString is not generic function");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_REGEXP_H_
