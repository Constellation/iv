#ifndef _IV_LV5_RUNTIME_ERROR_H_
#define _IV_LV5_RUNTIME_ERROR_H_
#include "conversions.h"
#include "ustring.h"
#include "ustringpiece.h"
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/jsstring.h"
#include "lv5/jserror.h"
#include "lv5/context.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

static const std::string kErrorSplitter(": ");

static inline JSString* ErrorMessageString(const Arguments& args,
                                           Error* error) {
  if (args.size() > 0) {
    const JSVal& msg = args[0];
    if (!msg.IsUndefined()) {
      return msg.ToString(args.ctx(), ERROR_WITH(error, NULL));
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

}  // namespace iv::lv5::runtime::detail

// section 15.11.1.1 Error(message)
// section 15.11.2.1 new Error(message)
inline JSVal ErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::New(args.ctx(), Error::User, message);
}

// section 15.11.4.4 Error.prototype.toString()
inline JSVal ErrorToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Error.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  Context* const ctx = args.ctx();
  if (obj.IsObject()) {
    JSString* name;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             ctx->Intern("name"),
                                             ERROR(error));
      if (target.IsUndefined()) {
        name = JSString::NewAsciiString(ctx, "Error");
      } else {
        name = target.ToString(ctx, ERROR(error));
      }
    }
    JSString* msg;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             ctx->Intern("message"),
                                             ERROR(error));
      if (target.IsUndefined()) {
        msg = JSString::NewEmptyString(ctx);
      } else {
        msg = target.ToString(ctx, ERROR(error));
      }
    }
    if (name->empty() && msg->empty()) {
      return JSString::NewAsciiString(ctx, "Error");
    }
    if (name->empty()) {
      return msg;
    }
    if (msg->empty()) {
      return name;
    }
    core::UString buffer;
    buffer.append(name->data(), name->size());
    buffer.append(detail::kErrorSplitter.begin(), detail::kErrorSplitter.end());
    buffer.append(msg->data(), msg->size());
    return JSString::New(ctx, buffer);
  }
  error->Report(Error::Type, "base must be object");
  return JSUndefined;
}

// section 15.11.6.1 EvalError
inline JSVal EvalErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::NewEvalError(args.ctx(), message);
}

// section 15.11.6.2 RangeError
inline JSVal RangeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::NewRangeError(args.ctx(), message);
}

// section 15.11.6.3 ReferenceError
inline JSVal ReferenceErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::NewReferenceError(args.ctx(), message);
}

// section 15.11.6.4 SyntaxError
inline JSVal SyntaxErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::NewSyntaxError(args.ctx(), message);
}

// section 15.11.6.5 TypeError
inline JSVal TypeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::NewTypeError(args.ctx(), message);
}

// section 15.11.6.6 URIError
inline JSVal URIErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = detail::ErrorMessageString(args, ERROR(error));
  return JSError::NewURIError(args.ctx(), message);
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_ERROR_H_
