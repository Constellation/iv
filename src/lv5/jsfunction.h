#ifndef _IV_LV5_JSFUNCTION_H_
#define _IV_LV5_JSFUNCTION_H_
#include <algorithm>
#include "ustringpiece.h"
#include "lv5/context_utils.h"
#include "lv5/jsobject.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
#include "lv5/jsast.h"
#include "lv5/lv5.h"
#include "lv5/railgun_fwd.h"
#include "lv5/teleporter_fwd.h"
namespace iv {
namespace lv5 {
namespace runtime {

JSVal GlobalEval(const Arguments& args, Error* error);

}  // namespace iv::lv5::runtime

class Context;
class JSNativeFunction;
class JSBoundFunction;
class Error;
class JSEnv;
class JSInterpreterScript;
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

  virtual JSVal Call(Arguments* args,
                     const JSVal& this_binding,
                     Error* error) = 0;

  virtual JSVal Construct(Arguments* args,
                          Error* error) = 0;

  virtual bool HasInstance(Context* ctx,
                           const JSVal& val, Error* e) {
    if (!val.IsObject()) {
      return false;
    }
    const JSVal got = Get(ctx, context::prototype_symbol(ctx), e);
    if (*e) {
      return false;
    }
    if (!got.IsObject()) {
      e->Report(Error::Type, "\"prototype\" is not object");
      return false;
    }
    const JSObject* const proto = got.object();
    const JSObject* obj = val.object()->prototype();
    while (obj) {
      if (obj == proto) {
        return true;
      } else {
        obj = obj->prototype();
      }
    }
    return false;
  }

  JSVal Get(Context* ctx,
            Symbol name, Error* e) {
    const JSVal val = JSObject::Get(ctx, name, e);
    if (*e) {
      return val;
    }
    if (name == context::caller_symbol(ctx) &&
        val.IsCallable() &&
        val.object()->AsCallable()->IsStrict()) {
      e->Report(Error::Type,
                "\"caller\" property is not accessible in strict code");
      return JSFalse;
    }
    return val;
  }

  virtual JSNativeFunction* AsNativeFunction() = 0;

  virtual teleporter::JSCodeFunction* AsCodeFunction() = 0;

  virtual JSBoundFunction* AsBoundFunction() = 0;

  virtual bool IsStrict() const = 0;

  virtual bool IsEvalFunction() const {
    return false;
  }
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

  JSVal Call(Arguments* args,
             const JSVal& this_binding,
             Error* error) {
    args->set_this_binding(this_binding);
    return func_(*args, error);
  }

  JSVal Construct(Arguments* args, Error* error) {
    args->set_this_binding(JSUndefined);
    return func_(*args, error);
  }

  teleporter::JSCodeFunction* AsCodeFunction() {
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
    obj->Initialize(ctx);
    return obj;
  }

  template<typename Func>
  static JSNativeFunction* NewPlain(Context* ctx,
                                    const Func& func, std::size_t n) {
    return new JSNativeFunction(ctx, func, n);
  }

  void Initialize(Context* ctx) {
    const Class& cls = context::Cls(ctx, "Function");
    set_class_name(cls.name);
    set_prototype(cls.prototype);
  }

 private:
  value_type func_;
};

class JSBoundFunction : public JSFunction {
 public:

  bool IsStrict() const {
    return false;
  }

  teleporter::JSCodeFunction* AsCodeFunction() {
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

  JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    using std::copy;
    ScopedArguments args_list(args->ctx(),
                              args->size() + arguments_.size(), ERROR(e));
    copy(args->begin(), args->end(),
         copy(arguments_.begin(), arguments_.end(), args_list.begin()));
    return target_->Call(&args_list, this_binding_, e);
  }

  JSVal Construct(Arguments* args, Error* e) {
    using std::copy;
    ScopedArguments args_list(args->ctx(),
                              args->size() + arguments_.size(), ERROR(e));
    copy(args->begin(), args->end(),
         copy(arguments_.begin(), arguments_.end(), args_list.begin()));
    return target_->Construct(&args_list, e);
  }

  bool HasInstance(Context* ctx,
                   const JSVal& val, Error* e) {
    return target_->HasInstance(ctx, val, e);
  }

  static JSBoundFunction* New(Context* ctx, JSFunction* target,
                              const JSVal& this_binding,
                              const Arguments& args) {
    return new JSBoundFunction(ctx, target, this_binding, args);
  }

 private:
  JSBoundFunction(Context* ctx,
                  JSFunction* target,
                  const JSVal& this_binding,
                  const Arguments& args)
    : target_(target),
      this_binding_(this_binding),
      arguments_(args.size() == 0 ? 0 : args.size() - 1) {
    if (args.size() > 0) {
      std::copy(args.begin() + 1, args.end(), arguments_.begin());
    }
    const uint32_t bound_args_size = arguments_.size();
    const Class& cls = context::Cls(ctx, "Function");
    set_class_name(cls.name);
    set_prototype(cls.prototype);
    // step 15
    if (target_->class_name() == cls.name) {
      // target [[Class]] is "Function"
      const JSVal length = target_->Get(ctx, context::length_symbol(ctx), NULL);
      assert(length.IsNumber());
      const uint32_t target_param_size = core::DoubleToUInt32(length.number());
      assert(target_param_size == length.number());
      const uint32_t len = (target_param_size >= bound_args_size) ?
          target_param_size - bound_args_size : 0;
      DefineOwnProperty(
          ctx, context::length_symbol(ctx),
          DataDescriptor(len,
                         PropertyDescriptor::NONE),
                         false, NULL);
    } else {
      DefineOwnProperty(
          ctx, context::length_symbol(ctx),
          DataDescriptor(0.0,
                         PropertyDescriptor::NONE),
                         false, NULL);
    }
    JSFunction* const throw_type_error = context::throw_type_error(ctx);
    DefineOwnProperty(ctx, context::caller_symbol(ctx),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, NULL);
    DefineOwnProperty(ctx, context::arguments_symbol(ctx),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, NULL);
  }

  JSFunction* target_;
  JSVal this_binding_;
  JSVals arguments_;
};

template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
class JSInlinedFunction : public JSFunction {
 public:
  typedef JSVal(*value_type)(const Arguments&, Error*);
  typedef JSInlinedFunction<func, n> this_type;

  explicit JSInlinedFunction(Context* ctx) {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(static_cast<double>(n),
                       PropertyDescriptor::NONE),
                       false, NULL);
    DefineOwnProperty(
        ctx, context::Intern(ctx, "name"),
        DataDescriptor(
            JSString::NewEmptyString(ctx),
            PropertyDescriptor::NONE),
            false, NULL);
  }

  JSInlinedFunction(Context* ctx, const Symbol& name) {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(static_cast<double>(n),
                       PropertyDescriptor::NONE),
                       false, NULL);
    DefineOwnProperty(
        ctx, context::Intern(ctx, "name"),
        DataDescriptor(
            JSString::New(ctx, context::GetSymbolString(ctx, name)),
            PropertyDescriptor::NONE),
            false, NULL);
  }

  JSVal Call(Arguments* args,
             const JSVal& this_binding,
             Error* error) {
    args->set_this_binding(this_binding);
    return func(*args, error);
  }

  JSVal Construct(Arguments* args, Error* error) {
    args->set_this_binding(JSUndefined);
    return func(*args, error);
  }

  teleporter::JSCodeFunction* AsCodeFunction() {
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

  static this_type* New(Context* ctx, const Symbol& name) {
    this_type* const obj = new this_type(ctx, name);
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

class JSVMFunction : public JSFunction {
 public:
  JSVMFunction(Context* ctx,
               const railgun::Code* code,
               JSScript* script,
               JSEnv* env);

  JSVal Call(Arguments* args,
             const JSVal& this_binding,
             Error* error);

  JSVal Construct(Arguments* args, Error* error);

  JSEnv* scope() const {
    return env_;
  }

  static JSVMFunction* New(Context* ctx,
                           const railgun::Code* code,
                           JSScript* script,
                           JSEnv* env) {
    return new JSVMFunction(ctx, code, script, env);
  }

  teleporter::JSCodeFunction* AsCodeFunction() {
    return NULL;
  }

  JSNativeFunction* AsNativeFunction() {
    return NULL;
  }

  JSBoundFunction* AsBoundFunction() {
    return NULL;
  }

  core::UStringPiece GetSource() const;

  bool IsStrict() const {
    return false;
  }

 private:
  const railgun::Code* code_;
  JSScript* script_;
  JSEnv* env_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
