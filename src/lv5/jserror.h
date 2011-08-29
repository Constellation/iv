#ifndef _IV_LV5_JSEXCEPTION_H_
#define _IV_LV5_JSEXCEPTION_H_
#include <cassert>
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/jsval.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {

class JSVal;
class JSString;

// fwd decls
class JSEvalError;
class JSRangeError;
class JSReferenceError;
class JSSyntaxError;
class JSTypeError;
class JSURIError;

class JSError : public JSObject {
 public:
  JSError(Context* ctx, Error::Code code, JSString* str)
    : JSObject(context::GetErrorMap(ctx)),
      code_(code) {
    if (str) {
      DefineOwnProperty(ctx, context::Intern(ctx, "message"),
                             DataDescriptor(str,
                                            PropertyDescriptor::CONFIGURABLE |
                                            PropertyDescriptor::ENUMERABLE |
                                            PropertyDescriptor::WRITABLE),
                                            false, NULL);
    }
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::Error
    };
    return &cls;
  }

  static inline JSVal Detail(Context* ctx, const Error* error);

  static JSError* New(Context* ctx, Error::Code code, JSString* str) {
    JSError* const error = new JSError(ctx, code, str);
    error->set_cls(JSError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::Error).prototype);
    return error;
  }

 protected:
  Error::Code code_;
  JSString* detail_;
};

class JSEvalError : public JSError {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::EvalError
    };
    return &cls;
  }

  static JSError* New(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Eval, str);
    error->set_cls(JSEvalError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::EvalError).prototype);
    return error;
  }
};

class JSRangeError : public JSError {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::RangeError
    };
    return &cls;
  }

  static JSError* New(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Range, str);
    error->set_cls(JSRangeError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::RangeError).prototype);
    return error;
  }
};

class JSReferenceError : public JSError {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::ReferenceError
    };
    return &cls;
  }

  static JSError* New(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Reference, str);
    error->set_cls(JSReferenceError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::ReferenceError).prototype);
    return error;
  }
};

class JSSyntaxError : public JSError {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::SyntaxError
    };
    return &cls;
  }

  static JSError* New(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Syntax, str);
    error->set_cls(JSSyntaxError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::SyntaxError).prototype);
    return error;
  }
};

class JSTypeError : public JSError {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::TypeError
    };
    return &cls;
  }

  static JSError* New(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::Type, str);
    error->set_cls(JSTypeError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::TypeError).prototype);
    return error;
  }
};

class JSURIError : public JSError {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::URIError
    };
    return &cls;
  }

  static JSError* New(Context* ctx, JSString* str) {
    JSError* const error = new JSError(ctx, Error::URI, str);
    error->set_cls(JSURIError::GetClass());
    error->set_prototype(context::GetClassSlot(ctx, Class::URIError).prototype);
    return error;
  }
};

JSVal JSError::Detail(Context* ctx, const Error* error) {
  assert(error && (error->code() != Error::Normal));
  switch (error->code()) {
    case Error::Eval:
      return JSEvalError::New(
          ctx, JSString::New(ctx, error->detail()));
    case Error::Range:
      return JSRangeError::New(
          ctx, JSString::New(ctx, error->detail()));
    case Error::Reference:
      return JSReferenceError::New(
          ctx, JSString::New(ctx, error->detail()));
    case Error::Syntax:
      return JSSyntaxError::New(
          ctx, JSString::New(ctx, error->detail()));
    case Error::Type:
      return JSTypeError::New(
          ctx, JSString::New(ctx, error->detail()));
    case Error::URI:
      return JSURIError::New(
          ctx, JSString::New(ctx, error->detail()));
    case Error::User:
      return error->value();
    default:
      UNREACHABLE();
      return JSUndefined;  // make compiler happy
  };
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSEXCEPTION_H_
