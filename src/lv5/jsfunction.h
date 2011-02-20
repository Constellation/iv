#ifndef _IV_LV5_JSFUNCTION_H_
#define _IV_LV5_JSFUNCTION_H_
#include "ustringpiece.h"
#include "lv5/context_utils.h"
#include "lv5/jsobject.h"
#include "lv5/arguments.h"
#include "lv5/jsast.h"
namespace iv {
namespace lv5 {
namespace runtime {
JSVal GlobalEval(const Arguments& args, Error* error);
}  // namespace iv::lv5::runtime

class Context;
class JSCodeFunction;
class JSNativeFunction;
class JSBoundFunction;
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
  virtual JSVal Call(Arguments& args,  // NOLINT
                     const JSVal& this_binding,
                     Error* error) = 0;
  virtual JSVal Construct(Arguments& args,  // NOLINT
                          Error* error) = 0;
  virtual bool HasInstance(Context* ctx,
                           const JSVal& val, Error* error);
  JSVal Get(Context* ctx,
            Symbol name, Error* error);
  virtual JSNativeFunction* AsNativeFunction() = 0;
  virtual JSCodeFunction* AsCodeFunction() = 0;
  virtual JSBoundFunction* AsBoundFunction() = 0;
  virtual bool IsStrict() const = 0;
  virtual bool IsEvalFunction() const {
    return false;
  }
  void Initialize(Context* ctx);
};

class JSCodeFunction : public JSFunction {
 public:
  JSCodeFunction(Context* ctx,
                 const FunctionLiteral* func,
                 JSScript* script,
                 JSEnv* env);
  JSVal Call(Arguments& args,  // NOLINT
             const JSVal& this_binding,
             Error* error);
  JSVal Construct(Arguments& args, Error* error);  // NOLINT
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
  JSBoundFunction* AsBoundFunction() {
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

  JSNativeFunction(Context* ctx, value_type func, std::size_t n)
    : func_(func) {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(n,
                       PropertyDescriptor::NONE),
                       false, NULL);
  }

  JSVal Call(Arguments& args,  // NOLINT
             const JSVal& this_binding,
             Error* error) {
    args.set_this_binding(this_binding);
    return func_(args, error);
  }

  JSVal Construct(Arguments& args, Error* error) {  // NOLINT
    args.set_this_binding(JSUndefined);
    return func_(args, error);
  }

  JSCodeFunction* AsCodeFunction() {
    return NULL;
  }

  JSNativeFunction* AsNativeFunction() {
    return this;
  }

  JSBoundFunction* AsBoundFunction() {
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

  void InitializeSimple(Context* ctx) {
    const Class& cls = context::Cls(ctx, "Function");
    set_class_name(cls.name);
    set_prototype(cls.prototype);
  }

  void Initialize(Context* ctx, value_type func, std::size_t n);

 private:
  value_type func_;
};

class JSBoundFunction : public JSFunction {
 public:
  JSBoundFunction(Context* ctx,
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
  JSBoundFunction* AsBoundFunction() {
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
  JSVal Call(Arguments& args, const JSVal& this_binding, Error* error);
  JSVal Construct(Arguments& args, Error* error);  // NOLINT
  bool HasInstance(Context* ctx,
                   const JSVal& val, Error* error);
  static JSBoundFunction* New(Context* ctx, JSFunction* target,
                              const JSVal& this_binding,
                              const Arguments& args);
 private:
  JSFunction* target_;
  JSVal this_binding_;
  JSVals arguments_;
};

template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
class JSInlinedFunction : public JSFunction {
 public:
  typedef JSVal(*value_type)(const Arguments&, Error*);
  typedef JSInlinedFunction<func, n> this_type;

  JSInlinedFunction(Context* ctx) {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(static_cast<double>(n),
                       PropertyDescriptor::NONE),
                       false, NULL);
  }

  JSVal Call(Arguments& args,  // NOLINT
             const JSVal& this_binding,
             Error* error) {
    args.set_this_binding(this_binding);
    return func(args, error);
  }

  JSVal Construct(Arguments& args, Error* error) {  // NOLINT
    args.set_this_binding(JSUndefined);
    return func(args, error);
  }

  JSCodeFunction* AsCodeFunction() {
    return NULL;
  }

  JSNativeFunction* AsNativeFunction() {
    return NULL;
  }

  JSBoundFunction* AsBoundFunction() {
    return NULL;
  }

  bool IsStrict() const {
    return false;
  }

  bool IsEvalFunction() const {
    return func == &runtime::GlobalEval;
  }

  value_type function() const {
    return func;
  }

  static this_type* New(Context* ctx) {
    this_type* const obj = new this_type(ctx);
    obj->Initialize(ctx);
    return obj;
  }

  static this_type* NewPlain(Context* ctx) {
    return new this_type(ctx);
  }

  void Initialize(Context* ctx) {
    const Class& cls = context::Cls(ctx, "Function");
    set_class_name(cls.name);
    set_prototype(cls.prototype);
  }
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
