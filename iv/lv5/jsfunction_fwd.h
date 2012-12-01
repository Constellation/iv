#ifndef IV_LV5_JSFUNCTION_FWD_H_
#define IV_LV5_JSFUNCTION_FWD_H_
#include <algorithm>
#include <iv/ustringpiece.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/error.h>
#include <iv/lv5/class.h>
#include <iv/lv5/map.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/radio/core_fwd.h>
#include <iv/lv5/jsstring_fwd.h>
namespace iv {
namespace lv5 {
typedef JSVal(*JSAPI)(const Arguments&, Error*);

class JSFunction : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSFunction, Function)

  enum Type {
    FUNCTION_NATIVE,
    FUNCTION_BOUND,
    FUNCTION_USER
  };

  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) = 0;

  virtual JSVal Construct(Arguments* args, Error* e) = 0;

  virtual bool HasInstance(Context* ctx, JSVal val, Error* e) {
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


  virtual JSAPI NativeFunction() const { return NULL; }

  virtual core::UStringPiece GetSource() const {
    static const core::UString kBlock =
        core::ToUString("() { \"[native code]\" }");
    return kBlock;
  }

  virtual core::UString GetName() const {
    return core::UString();
  }

  bool strict() const { return strict_; }
  Type function_type() const { return type_; }

  IV_LV5_INTERNAL_METHOD bool DefineOwnNonIndexedPropertySlotMethod(JSObject* obj,
                                                                    Context* ctx,
                                                                    Symbol name,
                                                                    const PropertyDescriptor& desc,
                                                                    Slot* slot,
                                                                    bool th, Error* e);
  IV_LV5_INTERNAL_METHOD JSVal GetNonIndexedSlotMethod(JSObject* obj, Context* ctx, Symbol name, Slot* slot, Error* e);

  Map* construct_map(Context* ctx, Error* e);
 protected:
  explicit JSFunction(Context* ctx, Type type, bool strict);
  JSFunction(Context* ctx, Map* map, Type type, bool strict);

  Type type_;
  bool strict_;
  Map* construct_map_;
};

class JSNativeFunction : public JSFunction {
 public:
  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
    args->set_this_binding(this_binding);
    return func_(*args, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    args->set_this_binding(JSUndefined);
    return func_(*args, e);
  }

  virtual JSAPI NativeFunction() const { return func_; }

  template<typename Func>
  static JSNativeFunction* New(Context* ctx,
                               const Func& func, std::size_t n,
                               Symbol name) {
    JSNativeFunction* const obj = new JSNativeFunction(ctx, func, n, name);
    return obj;
  }

  template<typename Func>
  static JSNativeFunction* NewPlain(Context* ctx,
                                    const Func& func, std::size_t n,
                                    Symbol name) {
    return new JSNativeFunction(ctx, func, n, name);
  }

 private:
  explicit JSNativeFunction(Context* ctx)
    : JSFunction(ctx, FUNCTION_NATIVE, false), func_() { }

  JSNativeFunction(Context* ctx, JSAPI func, uint32_t n, Symbol name)
    : JSFunction(ctx, FUNCTION_NATIVE, false),
      func_(func) {
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(n), ATTR::NONE), false, NULL);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(JSString::New(ctx, name), ATTR::NONE), false, NULL);
  }

  JSAPI func_;
};

class JSBoundFunction : public JSFunction {
 public:
  JSFunction* target() const { return target_; }

  JSVal this_binding() const { return this_binding_; }

  const JSVals& arguments() const { return arguments_; }

  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
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

  virtual bool HasInstance(Context* ctx, JSVal val, Error* e) {
    return target_->HasInstance(ctx, val, e);
  }

  static JSBoundFunction* New(Context* ctx, JSFunction* target,
                              JSVal this_binding,
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
                  JSVal this_binding,
                  const Arguments& args);

  JSFunction* target_;
  JSVal this_binding_;
  JSVals arguments_;
};

template<JSAPI func, uint32_t n>
class JSInlinedFunction : public JSFunction {
 public:
  typedef JSInlinedFunction<func, n> this_type;

  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
    args->set_this_binding(this_binding);
    return func(*args, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    args->set_this_binding(JSUndefined);
    return func(*args, e);
  }

  virtual JSAPI NativeFunction() const { return func; }

  static this_type* New(Context* ctx, Symbol name) {
    return new this_type(ctx, name);
  }

  static this_type* NewPlain(Context* ctx, Symbol name) {
    return new this_type(ctx, name);
  }

  static this_type* NewPlain(Context* ctx, Symbol name, Map* map) {
    return new this_type(ctx, name, map);
  }

 private:
  JSInlinedFunction(Context* ctx, Symbol name)
    : JSFunction(ctx, FUNCTION_NATIVE, false) {
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(n), ATTR::NONE), false, NULL);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(JSString::New(ctx, name), ATTR::NONE), false, NULL);
  }

  JSInlinedFunction(Context* ctx, Symbol name, Map* map)
    : JSFunction(ctx, map, FUNCTION_NATIVE, false) {
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(n), ATTR::NONE), false, NULL);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(JSString::New(ctx, name), ATTR::NONE), false, NULL);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSFUNCTION_FWD_H_
