#ifndef _IV_LV5_JSFUNCTION_H_
#define _IV_LV5_JSFUNCTION_H_
#include <algorithm>
#include "ustringpiece.h"
#include "lv5/error_check.h"
#include "lv5/context_utils.h"
#include "lv5/jsobject.h"
#include "lv5/error.h"
#include "lv5/arguments.h"
namespace iv {
namespace lv5 {

typedef JSVal(*JSAPI)(const Arguments&, Error*);

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

  virtual bool IsNativeFunction() const = 0;

  virtual JSAPI NativeFunction() const {
    return NULL;
  }

  virtual core::UStringPiece GetSource() const {
    static const core::UString kBlock =
        core::ToUString("() { \"[native code]\" }");
    return kBlock;
  }

  virtual core::UStringPiece GetName() const {
    return core::UStringPiece();
  }

  virtual bool IsStrict() const = 0;

  void Initialize(Context* ctx) {
    const Class& cls = context::Cls(ctx, "Function");
    set_class_name(cls.name);
    set_prototype(cls.prototype);
  }
};

class JSNativeFunction : public JSFunction {
 public:
  JSNativeFunction() : func_() { }

  JSNativeFunction(Context* ctx, JSAPI func, uint32_t n)
    : func_(func) {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(JSVal::UInt32(n),
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

  bool IsNativeFunction() const {
    return true;
  }

  bool IsStrict() const {
    return false;
  }

  virtual JSAPI NativeFunction() const {
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

 private:
  JSAPI func_;
};

class JSBoundFunction : public JSFunction {
 public:

  bool IsStrict() const {
    return false;
  }

  bool IsNativeFunction() const {
    return true;
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
    ScopedArguments args_list(args->ctx(),
                              args->size() + arguments_.size(),
                              IV_LV5_ERROR(e));
    std::copy(args->begin(), args->end(),
              std::copy(arguments_.begin(),
                        arguments_.end(), args_list.begin()));
    return target_->Call(&args_list, this_binding_, e);
  }

  JSVal Construct(Arguments* args, Error* e) {
    ScopedArguments args_list(args->ctx(),
                              args->size() + arguments_.size(),
                              IV_LV5_ERROR(e));
    std::copy(args->begin(), args->end(),
              std::copy(arguments_.begin(),
                        arguments_.end(), args_list.begin()));
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
    Error e;
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
      const JSVal length = target_->Get(ctx, context::length_symbol(ctx), &e);
      assert(length.IsNumber());
      const uint32_t target_param_size = length.ToUInt32(ctx, &e);
      assert(target_param_size == length.number());
      const uint32_t len = (target_param_size >= bound_args_size) ?
          target_param_size - bound_args_size : 0;
      DefineOwnProperty(
          ctx, context::length_symbol(ctx),
          DataDescriptor(JSVal::UInt32(len),
                         PropertyDescriptor::NONE),
                         false, &e);
    } else {
      DefineOwnProperty(
          ctx, context::length_symbol(ctx),
          DataDescriptor(JSVal::UInt32(0u),
                         PropertyDescriptor::NONE),
                         false, &e);
    }
    JSFunction* const throw_type_error = context::throw_type_error(ctx);
    DefineOwnProperty(ctx, context::caller_symbol(ctx),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, &e);
    DefineOwnProperty(ctx, context::arguments_symbol(ctx),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         PropertyDescriptor::NONE),
                      false, &e);
  }

  JSFunction* target_;
  JSVal this_binding_;
  JSVals arguments_;
};

template<JSAPI func, uint32_t n>
class JSInlinedFunction : public JSFunction {
 public:
  typedef JSInlinedFunction<func, n> this_type;

  explicit JSInlinedFunction(Context* ctx) {
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(JSVal::UInt32(n),
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
        DataDescriptor(JSVal::UInt32(n),
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

  bool IsNativeFunction() const {
    return true;
  }

  bool IsStrict() const {
    return false;
  }

  virtual JSAPI NativeFunction() const {
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
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSFUNCTION_H_
