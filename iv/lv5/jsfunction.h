#ifndef IV_LV5_JSFUNCTION_H_
#define IV_LV5_JSFUNCTION_H_
#include <algorithm>
#include <iv/ustringpiece.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/error.h>
#include <iv/lv5/class.h>
#include <iv/lv5/map.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {

typedef JSVal(*JSAPI)(const Arguments&, Error*);

class JSFunction : public JSObject {
 public:
  bool IsCallable() const {
    return true;
  }

  JSFunction* AsCallable() {
    return this;
  }

  virtual JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) = 0;

  virtual JSVal Construct(Arguments* args, Error* e) = 0;

  virtual bool HasInstance(Context* ctx, const JSVal& val, Error* e) {
    if (!val.IsObject()) {
      return false;
    }
    const JSVal got =
        Get(ctx, symbol::prototype(), IV_LV5_ERROR_WITH(e, false));
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

  virtual JSVal Get(Context* ctx, Symbol name, Error* e) {
    const JSVal val = JSObject::Get(ctx, name, IV_LV5_ERROR(e));
    if (name == symbol::caller() &&
        val.IsCallable() &&
        val.object()->AsCallable()->IsStrict()) {
      e->Report(Error::Type,
                "\"caller\" property is not accessible in strict code");
      return JSFalse;
    }
    return val;
  }

  virtual bool IsNativeFunction() const = 0;

  virtual bool IsBoundFunction() const = 0;

  virtual JSAPI NativeFunction() const { return NULL; }

  virtual core::UStringPiece GetSource() const {
    static const core::UString kBlock =
        core::ToUString("() { \"[native code]\" }");
    return kBlock;
  }

  virtual core::UString GetName() const {
    return core::UString();
  }

  virtual bool IsStrict() const = 0;

  void Initialize(Context* ctx) {
    set_cls(JSFunction::GetClass());
    set_prototype(context::GetClassSlot(ctx, Class::Function).prototype);
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Function",
      Class::Function
    };
    return &cls;
  }

 protected:
  explicit JSFunction(Context* ctx)
    : JSObject(context::GetFunctionMap(ctx)) { }
};

class JSNativeFunction : public JSFunction {
 public:
  virtual JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    args->set_this_binding(this_binding);
    return func_(*args, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    args->set_this_binding(JSUndefined);
    return func_(*args, e);
  }

  virtual bool IsNativeFunction() const { return true; }

  virtual bool IsBoundFunction() const { return false; }

  virtual bool IsStrict() const { return false; }

  virtual JSAPI NativeFunction() const { return func_; }

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
  explicit JSNativeFunction(Context* ctx)
    : JSFunction(ctx), func_() { }

  JSNativeFunction(Context* ctx, JSAPI func, uint32_t n)
    : JSFunction(ctx),
      func_(func) {
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(n), ATTR::NONE), false, NULL);
  }

  JSAPI func_;
};

class JSBoundFunction : public JSFunction {
 public:
  virtual bool IsStrict() const { return false; }

  virtual bool IsNativeFunction() const { return true; }

  virtual bool IsBoundFunction() const { return true; }

  JSFunction* target() const { return target_; }

  const JSVal& this_binding() const { return this_binding_; }

  const JSVals& arguments() const { return arguments_; }

  virtual JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    ScopedArguments args_list(args->ctx(),
                              args->size() + arguments_.size(),
                              IV_LV5_ERROR(e));
    std::copy(args->begin(), args->end(),
              std::copy(arguments_.begin(),
                        arguments_.end(), args_list.begin()));
    return target_->Call(&args_list, this_binding_, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    ScopedArguments args_list(args->ctx(),
                              args->size() + arguments_.size(),
                              IV_LV5_ERROR(e));
    std::copy(args->begin(), args->end(),
              std::copy(arguments_.begin(),
                        arguments_.end(), args_list.begin()));
    assert(args->IsConstructorCalled());
    args_list.set_constructor_call(true);
    return target_->Construct(&args_list, e);
  }

  virtual bool HasInstance(Context* ctx, const JSVal& val, Error* e) {
    return target_->HasInstance(ctx, val, e);
  }

  static JSBoundFunction* New(Context* ctx, JSFunction* target,
                              const JSVal& this_binding,
                              const Arguments& args) {
    return new JSBoundFunction(ctx, target, this_binding, args);
  }

  virtual void MarkChildren(radio::Core* core) {
    JSObject::MarkChildren(core);
    core->MarkCell(target_);
    core->MarkValue(this_binding_);
    std::for_each(arguments_.begin(),
                  arguments_.end(), radio::Core::Marker(core));
  }

 private:
  JSBoundFunction(Context* ctx,
                  JSFunction* target,
                  const JSVal& this_binding,
                  const Arguments& args)
    : JSFunction(ctx),
      target_(target),
      this_binding_(this_binding),
      arguments_(args.empty() ? 0 : args.size() - 1) {
    Error e;
    if (args.size() > 0) {
      std::copy(args.begin() + 1, args.end(), arguments_.begin());
    }
    const uint32_t bound_args_size = arguments_.size();
    set_cls(JSFunction::GetClass());
    set_prototype(context::GetClassSlot(ctx, Class::Function).prototype);
    // step 15
    if (target_->IsClass<Class::Function>()) {
      // target [[Class]] is "Function"
      const JSVal length = target_->Get(ctx, symbol::length(), &e);
      assert(length.IsNumber());
      const uint32_t target_param_size = length.ToUInt32(ctx, &e);
      assert(target_param_size == length.number());
      const uint32_t len = (target_param_size >= bound_args_size) ?
          target_param_size - bound_args_size : 0;
      DefineOwnProperty(
          ctx, symbol::length(),
          DataDescriptor(JSVal::UInt32(len), ATTR::NONE), false, &e);
    } else {
      DefineOwnProperty(
          ctx, symbol::length(),
          DataDescriptor(JSVal::UInt32(0u), ATTR::NONE), false, &e);
    }
    JSFunction* const throw_type_error = context::throw_type_error(ctx);
    DefineOwnProperty(ctx, symbol::caller(),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         ATTR::NONE),
                      false, &e);
    DefineOwnProperty(ctx, symbol::arguments(),
                      AccessorDescriptor(throw_type_error,
                                         throw_type_error,
                                         ATTR::NONE),
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

  virtual JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    args->set_this_binding(this_binding);
    return func(*args, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    args->set_this_binding(JSUndefined);
    return func(*args, e);
  }

  virtual bool IsNativeFunction() const { return true; }

  virtual bool IsBoundFunction() const { return false; }

  virtual bool IsStrict() const { return false; }

  virtual JSAPI NativeFunction() const { return func; }

  static this_type* New(Context* ctx, const Symbol& name) {
    this_type* const obj = new this_type(ctx, name);
    obj->Initialize(ctx);
    return obj;
  }

  static this_type* NewPlain(Context* ctx, const Symbol& name) {
    return new this_type(ctx, name);
  }

 private:
  JSInlinedFunction(Context* ctx, const Symbol& name)
    : JSFunction(ctx) {
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(n), ATTR::NONE), false, NULL);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(JSString::New(ctx, name), ATTR::NONE), false, NULL);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSFUNCTION_H_
