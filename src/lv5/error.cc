#include "context.h"
#include "error.h"
#include "jsexception.h"

namespace iv {
namespace lv5 {

void Error::Report(Code code, const core::StringPiece& str) {
  code_ = code;
  str.CopyToString(&detail_);
}

void Error::Report(const JSVal& val) {
  code_ = User;
  value_ = val;
}

void Error::Clear() {
  code_ = Normal;
}

JSVal Error::Detail(Context* ctx) const {
  assert(code_ != Normal);
  switch (code_) {
    case Native:
      return JSError::NewNativeError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case Eval:
      return JSError::NewEvalError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case Range:
      return JSError::NewRangeError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case Reference:
      return JSError::NewReferenceError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case Syntax:
      return JSError::NewSyntaxError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case Type:
      return JSError::NewTypeError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case URI:
      return JSError::NewURIError(
          ctx, JSString::NewAsciiString(ctx, detail_));
    case User:
      return value_;
    default:
      UNREACHABLE();
      return JSUndefined;  // make compiler happy
  };
}

} }  // namespace iv::lv5
