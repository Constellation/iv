#ifndef _IV_LV5_JSFUNCTION_H_
#define _IV_LV5_JSFUNCTION_H_
#include <tr1/functional>
#include "jsobject.h"
#include "jsenv.h"
#include "arguments.h"
#include "ustringpiece.h"
namespace iv {
namespace lv5 {

class Context;
class JSCodeFunction;
class JSNativeFunction;
class Error;
class JSFunction : public JSObject {
 public:
  JSFunction() : JSObject() { }
  bool IsCallable() const {
    return true;
  }
  JSFunction* AsCallable() {
    return this;
  }
  virtual JSVal Call(const Arguments& args,
                     Error* error) = 0;
  bool HasInstance(Context* ctx,
                   const JSVal& val, Error* error);
  JSVal Get(Context* ctx,
            Symbol name, Error* error);
  virtual JSCodeFunction* AsCodeFunction() = 0;
  virtual JSNativeFunction* AsNativeFunction() = 0;
  virtual bool IsStrict() const = 0;
  void Initialize(Context* ctx);
};

class JSCodeFunction : public JSFunction {
 public:
  JSCodeFunction(Context* ctx,
                 const core::FunctionLiteral* func, JSEnv* env);
  JSVal Call(const Arguments& args,
             Error* error);
  JSEnv* scope() const {
    return env_;
  }
  const core::FunctionLiteral* code() const {
    return function_;
  }
  static JSCodeFunction* New(Context* ctx,
                             const core::FunctionLiteral* func, JSEnv* env);
  JSCodeFunction* AsCodeFunction() {
    return this;
  }
  JSNativeFunction* AsNativeFunction() {
    return NULL;
  }
  core::UStringPiece GetSource() const {
    return function_->GetSource();
  }
  const core::Identifier* name() const {
    return function_->name();
  }
  bool IsStrict() const {
    return function_->strict();
  }
 private:
  const core::FunctionLiteral* function_;
  JSEnv* env_;
};

class JSNativeFunction : public JSFunction {
 public:
  typedef JSVal(*value_type)(const Arguments&, Error*);
  JSNativeFunction() : func_() { }
  JSNativeFunction(Context* ctx, value_type func, std::size_t n);
  JSVal Call(const Arguments& args,
             Error* error);
  JSCodeFunction* AsCodeFunction() {
    return NULL;
  }
  JSNativeFunction* AsNativeFunction() {
    return this;
  }
  bool IsStrict() const {
    return false;
  }

  template<typename Func>
  static JSNativeFunction* New(Context* ctx, const Func& func, std::size_t n) {
    JSNativeFunction* const obj = new JSNativeFunction(ctx, func, n);
    static_cast<JSFunction*>(obj)->Initialize(ctx);
    return obj;
  }

  void Initialize(Context* ctx, value_type func, std::size_t n);

 private:
  value_type func_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
