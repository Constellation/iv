#ifndef _IV_LV5_JSFUNCTION_H_
#define _IV_LV5_JSFUNCTION_H_
#include <tr1/functional>
#include "ustringpiece.h"
#include "jsobject.h"
#include "arguments.h"
#include "jsast.h"
namespace iv {
namespace lv5 {

class Context;
class JSCodeFunction;
class JSNativeFunction;
class JSBindedFunction;
class Error;
class JSEnv;
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
  virtual ~JSFunction() { }
  virtual JSVal Call(const Arguments& args, Error* error) = 0;
  virtual bool HasInstance(Context* ctx,
                           const JSVal& val, Error* error);
  JSVal Get(Context* ctx,
            Symbol name, Error* error);
  virtual JSNativeFunction* AsNativeFunction() = 0;
  virtual JSCodeFunction* AsCodeFunction() = 0;
  virtual JSBindedFunction* AsBindedFunction() = 0;
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
  JSBindedFunction* AsBindedFunction() {
    return NULL;
  }
  core::UStringPiece GetSource() const;
  const core::Maybe<Identifier> name() const {
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
  JSBindedFunction* AsBindedFunction() {
    return NULL;
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

  void Initialize(Context* ctx, value_type func, std::size_t n);

 private:
  value_type func_;
};

class JSBindedFunction : public JSFunction {
 public:
  JSBindedFunction(Context* ctx,
                   JSFunction* target,
                   const JSVal& this_binding,
                   const Arguments& args);
  bool IsStrict() const {
    return false;
  }
  JSCodeFunction* AsCodeFunction() {
    return NULL;
  }
  JSNativeFunction* AsNativeFunction() {
    return NULL;
  }
  JSBindedFunction* AsBindedFunction() {
    return this;
  }
  JSFunction* target() const {
    return target_;
  }
  const JSVal& this_binding() const {
    return this_binding_;
  }
  const JSVals& arguments() const {
    return arguments_;
  }
  JSVal Call(const Arguments& args, Error* error);
  bool HasInstance(Context* ctx,
                   const JSVal& val, Error* error);
  static JSBindedFunction* New(Context* ctx, JSFunction* target,
                               const JSVal& this_binding,
                               const Arguments& args);
 private:
  JSFunction* target_;
  JSVal this_binding_;
  JSVals arguments_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
