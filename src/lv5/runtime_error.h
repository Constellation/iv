#ifndef IV_LV5_RUNTIME_ERROR_H_
#define IV_LV5_RUNTIME_ERROR_H_
#include "conversions.h"
#include "ustring.h"
#include "ustringpiece.h"
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
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

static inline JSString* ErrorMessageString(const Arguments& args, Error* e) {
  if (!args.empty()) {
    const JSVal& msg = args.front();
    if (!msg.IsUndefined()) {
      return msg.ToString(args.ctx(), IV_LV5_ERROR_WITH(e, NULL));
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

}  // namespace detail

// section 15.11.1.1 Error(message)
// section 15.11.2.1 new Error(message)
inline JSVal ErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSError::New(args.ctx(), Error::User, message);
}

// section 15.11.4.4 Error.prototype.toString()
inline JSVal ErrorToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Error.prototype.toString", args, e);
  const JSVal& obj = args.this_binding();
  Context* const ctx = args.ctx();
  if (obj.IsObject()) {
    JSString* name;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             context::Intern(ctx, "name"),
                                             IV_LV5_ERROR(e));
      if (target.IsUndefined()) {
        name = JSString::NewAsciiString(ctx, "Error");
      } else {
        name = target.ToString(ctx, IV_LV5_ERROR(e));
      }
    }
    JSString* msg;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             context::Intern(ctx, "message"),
                                             IV_LV5_ERROR(e));
      if (target.IsUndefined()) {
        msg = JSString::NewEmptyString(ctx);
      } else {
        msg = target.ToString(ctx, IV_LV5_ERROR(e));
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
    StringBuilder builder;
    builder.Append(*name);
    builder.Append(": ");
    builder.Append(*msg);
    return builder.Build(ctx);
  }
  e->Report(Error::Type, "base must be object");
  return JSUndefined;
}

// section 15.11.6.1 EvalError
inline JSVal EvalErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSEvalError::New(args.ctx(), message);
}

// section 15.11.6.2 RangeError
inline JSVal RangeErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSRangeError::New(args.ctx(), message);
}

// section 15.11.6.3 ReferenceError
inline JSVal ReferenceErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSReferenceError::New(args.ctx(), message);
}

// section 15.11.6.4 SyntaxError
inline JSVal SyntaxErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSSyntaxError::New(args.ctx(), message);
}

// section 15.11.6.5 TypeError
inline JSVal TypeErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSTypeError::New(args.ctx(), message);
}

// section 15.11.6.6 URIError
inline JSVal URIErrorConstructor(const Arguments& args, Error* e) {
  JSString* message = detail::ErrorMessageString(args, IV_LV5_ERROR(e));
  return JSURIError::New(args.ctx(), message);
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_ERROR_H_
