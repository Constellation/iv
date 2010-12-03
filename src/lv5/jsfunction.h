#ifndef _IV_LV5_JSFUNCTION_H_
#define _IV_LV5_JSFUNCTION_H_
#include <tr1/functional>
#include "jsobject.h"
#include "jsenv.h"
#include "arguments.h"
#include "ustringpiece.h"
#include "jsast.h"
namespace iv {
namespace lv5 {

class Context;
class JSCodeFunction;
class JSNativeFunction;
class Error;
class JSScript;

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
  virtual JSNativeFunction* AsNativeFunction() = 0;
  virtual JSCodeFunction* AsCodeFunction() = 0;
  virtual bool IsStrict() const = 0;
  void Initialize(Context* ctx);
};

class JSCodeFunction : public JSFunction {
 public:
  JSCodeFunction(Context* ctx,
                 const FunctionLiteral* func,
                 JSScript* script,
                 JSEnv* env);
  JSVal Call(const Arguments& args, Error* error);
  JSEnv* scope() const {
    return env_;
  }
  const FunctionLiteral* code() const {
    return function_;
  }

  static JSCodeFunction* New(Context* ctx,
                             const FunctionLiteral* func,
                             JSScript* script,
                             JSEnv* env) {
    JSCodeFunction* const obj =
        new JSCodeFunction(ctx, func, script, env);
    obj->Initialize(ctx);
    return obj;
  }

  JSCodeFunction* AsCodeFunction() {
    return this;
  }
  JSNativeFunction* AsNativeFunction() {
    return NULL;
  }
  core::UStringPiece GetSource() const;
  const Identifier* name() const {
    return function_->name();
  }
  bool IsStrict() const {
    return function_->strict();
  }
 private:
  const FunctionLiteral* function_;
  JSScript* script_;
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
  value_type function() const {
    return func_;
  }

  template<typename Func>
  static JSNativeFunction* New(Context* ctx, const Func& func, std::size_t n) {
    JSNativeFunction* const obj = new JSNativeFunction(ctx, func, n);
    obj->InitializeSimple(ctx);
    return obj;
  }

  template<typename Func>
  static JSNativeFunction* NewPlain(Context* ctx,
                                    const Func& func, std::size_t n) {
    return new JSNativeFunction(ctx, func, n);
  }

  void InitializeSimple(Context* ctx);

  void Initialize(Context* ctx) {
    JSFunction::Initialize(ctx);
  }
  void Initialize(Context* ctx, value_type func, std::size_t n);

 private:
  value_type func_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
