#ifndef IV_LV5_JSEXCEPTION_H_
#define IV_LV5_JSEXCEPTION_H_
#include <cassert>
#include <iv/ustring.h>
#include <iv/unicode.h>
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
  IV_LV5_DEFINE_JSCLASS(Error)

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
      DefineOwnProperty(ctx, symbol::message(),
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

// Error function implementations (error.h)

inline bool Error::RequireMaterialize(Context* ctx) const {
  if (code() == Error::User) {
    if (value().IsObject() && value().object()->IsClass<Class::Error>()) {
      JSError* error = static_cast<JSError*>(value().object());
      if (error->HasProperty(ctx, symbol::stack())) {
        return false;
      }
      return !stack();
    }
    return false;
  }
  return !stack();
}

inline JSVal Error::Detail(Context* ctx) {
  assert(code() != Error::Normal);
  JSError* error = NULL;
  switch (code()) {
    case Error::Eval: {
      error = JSEvalError::New(ctx, JSString::New(ctx, detail()));
      break;
    }

    case Error::Range: {
      error = JSRangeError::New(ctx, JSString::New(ctx, detail()));
      break;
    }

    case Error::Reference: {
      error = JSReferenceError::New(ctx, JSString::New(ctx, detail()));
      break;
    }

    case Error::Syntax: {
      error = JSSyntaxError::New(ctx, JSString::New(ctx, detail()));
      break;
    }

    case Error::Type: {
      error = JSTypeError::New(ctx, JSString::New(ctx, detail()));
      break;
    }

    case Error::URI: {
      error = JSURIError::New(ctx, JSString::New(ctx, detail()));
      break;
    }

    case Error::User: {
      if (!value().IsObject() || !value().object()->IsClass<Class::Error>()) {
        Clear();
        return value();
      }
      error = static_cast<JSError*>(value().object());
      break;
    }

    default:
      UNREACHABLE();
      return JSUndefined;  // make compiler happy
  };
  assert(error);
  if (!error->HasProperty(ctx, symbol::stack())) {
    core::UString dump;
    if (stack()) {
      for (Stack::const_iterator it = stack()->begin(),
           last = stack()->end(); it != last; ++it) {
        dump.append(*it);
        dump.push_back('\n');
      }
    }
    Clear();
    if (!dump.empty()) {
      error->DefineOwnProperty(
          ctx, symbol::stack(),
          DataDescriptor(JSString::New(ctx, dump), ATTR::NONE),
          false, this);
    }
    if (*this) {
      Clear();
    }
  } else {
    Clear();
  }
  return error;
}

inline void Error::Dump(Context* ctx, FILE* out) {
  assert(*this);
  core::UString dump;
  if (stack()) {
    for (Stack::const_iterator it = stack()->begin(),
         last = stack()->end(); it != last; ++it) {
      dump.append(*it);
      dump.push_back('\n');
    }
  }
  const JSVal result = Detail(ctx);
  const iv::lv5::JSString* const str = result.ToString(ctx, this);
  if (!*this) {
    std::fprintf(out, "%s\n", str->GetUTF8().c_str());
    core::unicode::FPutsUTF16(out, dump);
  } else {
    Clear();
    std::fprintf(out, "%s\n", "ToString failed");
    core::unicode::FPutsUTF16(out, dump);
  }
  assert(!*this);
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSEXCEPTION_H_
