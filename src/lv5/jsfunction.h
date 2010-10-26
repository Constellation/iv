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
 protected:
  static void SetClass(Context* ctx, JSObject* obj);
};

class JSCodeFunction : public JSFunction {
 public:
  JSCodeFunction(const core::FunctionLiteral* func, JSEnv* env);
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
  explicit JSNativeFunction(value_type func)
    : func_(func) {
  }
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
  static JSNativeFunction* New(Context* ctx, const Func& func) {
    JSNativeFunction* const obj = new JSNativeFunction(func);
    SetClass(ctx, obj);
    return obj;
  }

 private:
  value_type func_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
