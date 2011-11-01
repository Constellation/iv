#ifndef IV_LV5_JSEXCEPTION_H_
#define IV_LV5_JSEXCEPTION_H_
#include <cassert>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context_utils.h>

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
  static const Class* GetClass() {
    static const Class cls = {
      "Error",
      Class::Error
    };
    return &cls;
  }

  static JSVal Detail(Context* ctx, const Error* e);

  static JSError* New(Context* ctx, Error::Code code, JSString* str) {
    JSError* const err = new JSError(ctx, code, str);
    err->set_cls(JSError::GetClass());
    err->set_prototype(context::GetClassSlot(ctx, Class::Error).prototype);
    return err;
  }

  JSError(Context* ctx, Error::Code code, JSString* str)
    : JSObject(context::GetErrorMap(ctx)),
      code_(code) {
    if (str) {
      DefineOwnProperty(ctx,
                        context::Intern(ctx, "message"),
                        DataDescriptor(str, ATTR::C | ATTR::W), false, NULL);
    }
  }

 private:
  Error::Code code_;
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
    JSError* const err = new JSError(ctx, Error::Eval, str);
    err->set_cls(JSEvalError::GetClass());
    err->set_prototype(context::GetClassSlot(ctx, Class::EvalError).prototype);
    return err;
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
    JSError* const err = new JSError(ctx, Error::Range, str);
    err->set_cls(JSRangeError::GetClass());
    err->set_prototype(context::GetClassSlot(ctx, Class::RangeError).prototype);
    return err;
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
    JSError* const err = new JSError(ctx, Error::Reference, str);
    err->set_cls(JSReferenceError::GetClass());
    err->set_prototype(
        context::GetClassSlot(ctx, Class::ReferenceError).prototype);
    return err;
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
    JSError* const err = new JSError(ctx, Error::Syntax, str);
    err->set_cls(JSSyntaxError::GetClass());
    err->set_prototype(
        context::GetClassSlot(ctx, Class::SyntaxError).prototype);
    return err;
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
    JSError* const err = new JSError(ctx, Error::Type, str);
    err->set_cls(JSTypeError::GetClass());
    err->set_prototype(context::GetClassSlot(ctx, Class::TypeError).prototype);
    return err;
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
    JSError* const err = new JSError(ctx, Error::URI, str);
    err->set_cls(JSURIError::GetClass());
    err->set_prototype(context::GetClassSlot(ctx, Class::URIError).prototype);
    return err;
  }
};

inline JSVal JSError::Detail(Context* ctx, const Error* e) {
  assert(e&& (e->code() != Error::Normal));
  switch (e->code()) {
    case Error::Eval:
      return JSEvalError::New(
          ctx, JSString::New(ctx, e->detail()));
    case Error::Range:
      return JSRangeError::New(
          ctx, JSString::New(ctx, e->detail()));
    case Error::Reference:
      return JSReferenceError::New(
          ctx, JSString::New(ctx, e->detail()));
    case Error::Syntax:
      return JSSyntaxError::New(
          ctx, JSString::New(ctx, e->detail()));
    case Error::Type:
      return JSTypeError::New(
          ctx, JSString::New(ctx, e->detail()));
    case Error::URI:
      return JSURIError::New(
          ctx, JSString::New(ctx, e->detail()));
    case Error::User:
      return e->value();
    default:
      UNREACHABLE();
      return JSUndefined;  // make compiler happy
  };
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSEXCEPTION_H_
