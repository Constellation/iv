#ifndef _IV_LV5_RUNTIME_ARRAY_H_
#define _IV_LV5_RUNTIME_ARRAY_H_
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsarray.h"
#include "jsstring.h"
#include "conversions.h"
#include "runtime_object.h"

namespace iv {
namespace lv5 {
namespace runtime {

// section 15.4.1.1 Array([item0 [, item1 [, ...]]])
// section 15.4.2.1 new Array([item0 [, item1 [, ...]]])
// section 15.4.2.2 new Array(len)
inline JSVal ArrayConstructor(const Arguments& args, Error* error) {
// TODO(Constellation) this is mock function
  return JSArray::New(args.ctx());
}

// section 15.4.4.2 Array.prototype.toString()
inline JSVal ArrayToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.toString", args, error);
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  const JSVal join = obj->Get(args.ctx(),
                              args.ctx()->Intern("join"), ERROR(error));
  if (join.IsCallable()) {
    return join.object()->AsCallable()->Call(
        Arguments(args.ctx(), obj), ERROR(error));
  } else {
    return ObjectToString(Arguments(args.ctx(), obj), ERROR(error));
  }
}

// section 15.4.4.5 Array.prototype.join(separator)
inline JSVal ArrayToJoin(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.join", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(error));
  const double val = length.ToNumber(ctx, ERROR(error));
  const uint32_t len = core::DoubleToUInt32(val);
  JSString* separator;
  if (args.size() > 0 && !args[0].IsUndefined()) {
    separator = args[0].ToString(ctx, ERROR(error));
  } else {
    separator = JSString::NewAsciiString(ctx, ",");
  }
  if (len == 0) {
    return JSString::NewEmptyString(ctx);
  }
  JSStringBuilder builder(ctx);
  {
    const JSVal element0 = obj->Get(ctx, ctx->Intern("0"), ERROR(error));
    if (!element0.IsUndefined() && !element0.IsNull()) {
      const JSString* const str = element0.ToString(ctx, ERROR(error));
      builder.Append(*str);
    }
  }
  uint32_t k = 1;
  std::tr1::array<char, 30> buf;
  while (k < len) {
    builder.Append(*separator);
    const int num = std::snprintf(buf.data(), buf.size(), "%lu",
                                  static_cast<unsigned long>(k));  // NOLINT
    const JSVal element = obj->Get(
        ctx,
        ctx->Intern(core::StringPiece(buf.data(), num)),
        ERROR(error));
    if (!element.IsUndefined() && !element.IsNull()) {
      const JSString* const str = element.ToString(ctx, ERROR(error));
      builder.Append(*str);
    }
    k += 1;
  }
  return builder.Build();
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_ARRAY_H_
