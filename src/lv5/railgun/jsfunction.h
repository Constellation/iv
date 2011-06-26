#ifndef _IV_LV5_RAILGUN_JSFUNCTION_H_
#define _IV_LV5_RAILGUN_JSFUNCTION_H_
#include "ustringpiece.h"
#include "lv5/arguments.h"
#include "lv5/jsenv.h"
#include "lv5/jsscript.h"
#include "lv5/jsfunction.h"
#include "lv5/jsarguments.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/code.h"
#include "lv5/railgun/jsscript.h"
#include "lv5/railgun/context_fwd.h"
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/frame.h"
namespace iv {
namespace lv5 {
namespace railgun {

class JSVMFunction : public JSFunction {
 public:
  JSVMFunction(Context* ctx,
               railgun::Code* code, JSEnv* env)
    : code_(code),
      env_(env) {
    Error e;
    DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(
            JSVal::UInt32(static_cast<uint32_t>(code->params().size())),
            PropertyDescriptor::NONE),
        false, &e);
    // section 13.2 Creating Function Objects
    const Class& cls = context::Cls(ctx, "Function");
    set_class_name(cls.name);
    set_prototype(cls.prototype);

    JSObject* const proto = JSObject::New(ctx);
    proto->DefineOwnProperty(
        ctx, context::constructor_symbol(ctx),
        DataDescriptor(this,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, &e);
    DefineOwnProperty(
        ctx, context::prototype_symbol(ctx),
        DataDescriptor(proto,
                       PropertyDescriptor::WRITABLE),
        false, &e);
    core::UStringPiece name = GetName();
    if (!name.empty()) {
      DefineOwnProperty(
          ctx, context::Intern(ctx, "name"),
          DataDescriptor(JSString::New(ctx, name),
                         PropertyDescriptor::NONE),
          false, &e);
    }
    if (code->strict()) {
      JSFunction* const throw_type_error = ctx->throw_type_error();
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
  }

  JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    VM* const vm = ctx->vm();
    args->set_this_binding(this_binding);
    const std::pair<JSVal, VM::Status> res = vm->Execute(*args, this);
    if (res.second == VM::THROW) {
      e->Report(res.first);
      return JSEmpty;
    } else {
      return res.first;
    }
  }

  JSVal Construct(Arguments* args, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    JSObject* const obj = JSObject::New(ctx);
    const JSVal proto = Get(ctx, context::prototype_symbol(ctx), IV_LV5_ERROR(e));
    if (proto.IsObject()) {
      obj->set_prototype(proto.object());
    }
    return Call(args, obj, IV_LV5_ERROR(e));
  }

  JSEnv* scope() const {
    return env_;
  }

  Code* code() const {
    return code_;
  }

  static JSVMFunction* New(Context* ctx,
                           railgun::Code* code, JSEnv* env) {
    return new JSVMFunction(ctx, code, env);
  }

  void InstantiateBindings(Context* ctx, Frame* frame, Error* e) {
    Instantiate(ctx, code_, frame, false, false, this, IV_LV5_ERROR_VOID(e));
    JSDeclEnv* env = static_cast<JSDeclEnv*>(frame->variable_env());
    const FunctionLiteral::DeclType type = code_->decl_type();
    if (type == FunctionLiteral::STATEMENT ||
        (type == FunctionLiteral::EXPRESSION && code_->HasName())) {
      const Symbol& name = code_->name();
      if (!env->HasBinding(ctx, name)) {
        env->CreateImmutableBinding(name);
        env->InitializeImmutableBinding(name, this);
      }
    }
  }

  bool IsNativeFunction() const {
    return false;
  }

  core::UStringPiece GetSource() const {
    const std::size_t start_pos = code_->start_position();
    const std::size_t end_pos = code_->end_position();
    return code_->script()->SubString(start_pos, end_pos - start_pos);
  }

  bool IsStrict() const {
    return code_->strict();
  }

 private:
  Code* code_;
  JSEnv* env_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_JSFUNCTION_H_
