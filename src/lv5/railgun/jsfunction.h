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
        ctx, symbol::length,
        DataDescriptor(
            JSVal::UInt32(static_cast<uint32_t>(code->params().size())),
            PropertyDescriptor::NONE),
        false, &e);
    // section 13.2 Creating Function Objects
    set_cls(JSFunction::GetClass());
    set_prototype(context::GetClassSlot(ctx, Class::Function).prototype);

    JSObject* const proto = JSObject::New(ctx);
    proto->DefineOwnProperty(
        ctx, symbol::constructor,
        DataDescriptor(this,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, &e);
    DefineOwnProperty(
        ctx, symbol::prototype,
        DataDescriptor(proto,
                       PropertyDescriptor::WRITABLE),
        false, &e);
    if (code->HasName()) {
      DefineOwnProperty(
          ctx, context::Intern(ctx, "name"),
          DataDescriptor(
              JSString::New(ctx, context::GetSymbolString(ctx, code->name())),
              PropertyDescriptor::NONE),
          false, &e);
    }
    if (code->strict()) {
      JSFunction* const throw_type_error = ctx->throw_type_error();
      DefineOwnProperty(ctx, symbol::caller,
                        AccessorDescriptor(throw_type_error,
                                           throw_type_error,
                                           PropertyDescriptor::NONE),
                        false, &e);
      DefineOwnProperty(ctx, symbol::arguments,
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
    const std::pair<JSVal, VM::Status> res =
        vm->Execute(*args, this, IV_LV5_ERROR(e));
    assert(res.second != VM::THROW);
    if (res.second == VM::RETURN) {
      return res.first;
    } else {
      if (args->IsConstructorCalled()) {
        return res.first;
      } else {
        return JSUndefined;
      }
    }
  }

  JSVal Construct(Arguments* args, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    JSObject* const obj = JSObject::New(ctx);
    const JSVal proto = Get(ctx, symbol::prototype, IV_LV5_ERROR(e));
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
    // step 1
    JSVal this_value = frame->GetThis();
    if (!code_->strict()) {
      if (this_value.IsUndefined() || this_value.IsNull()) {
        this_value.set_value(ctx->global_obj());
      } else if (!this_value.IsObject()) {
        JSObject* const obj = this_value.ToObject(ctx, IV_LV5_ERROR_VOID(e));
        this_value.set_value(obj);
      }
    }
    frame->set_this_binding(this_value);
    JSDeclEnv* env = static_cast<JSDeclEnv*>(frame->variable_env());
    JSVal* args = frame->arguments_begin();
    const std::size_t arg_count = frame->argc_;
    for (Code::Decls::const_iterator it = code_->decls().begin(),
         last = code_->decls().end(); it != last; ++it) {
      const Code::Decl& decl = *it;
      const Symbol sym = std::get<0>(decl);
      const Code::DeclType type = std::get<1>(decl);
      const bool immutable = std::get<2>(decl);
      if (type == Code::PARAM) {
        const std::size_t param = std::get<3>(decl);
        env->CreateMutableBinding(ctx, sym, false, IV_LV5_ERROR_VOID(e));
        if (param >= arg_count) {
          env->SetMutableBinding(ctx, sym, JSUndefined, code_->strict(), IV_LV5_ERROR_VOID(e));
        } else {
          env->SetMutableBinding(ctx, sym, args[param], code_->strict(), IV_LV5_ERROR_VOID(e));
        }
      } else if (type == Code::PARAM_LOCAL) {
        // initialize local value
        const std::size_t param = std::get<3>(decl);
        const std::size_t target = std::get<4>(decl);
        if (param >= arg_count) {
          frame->GetLocal()[target] = JSUndefined;
        } else {
          frame->GetLocal()[target] = args[param];
        }
      } else if (type == Code::FDECL) {
        env->CreateMutableBinding(ctx, sym, false, IV_LV5_ERROR_VOID(e));
      } else if (type == Code::ARGUMENTS) {
        JSArguments* const args_obj =
            JSArguments::New(ctx, this,
                             code_->params(),
                             frame->arguments_rbegin(),
                             frame->arguments_rend(), env,
                             code_->strict(), IV_LV5_ERROR_VOID(e));
        if (immutable) {
          env->CreateImmutableBinding(sym);
          env->InitializeImmutableBinding(sym, args_obj);
        } else {
          env->CreateMutableBinding(ctx, sym, false, IV_LV5_ERROR_VOID(e));
          env->SetMutableBinding(ctx, sym, args_obj, false, IV_LV5_ERROR_VOID(e));
        }
      } else if (type == Code::VAR) {
        env->CreateMutableBinding(ctx, sym, false, IV_LV5_ERROR_VOID(e));
        env->SetMutableBinding(ctx, sym, JSUndefined, code_->strict(), IV_LV5_ERROR_VOID(e));
      } else if (type == Code::FEXPR) {
        env->CreateImmutableBinding(sym);
        env->InitializeImmutableBinding(sym, this);
      }
    }
  }

  bool IsNativeFunction() const {
    return false;
  }

  bool IsBoundFunction() const {
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
