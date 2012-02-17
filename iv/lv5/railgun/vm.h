#ifndef IV_LV5_RAILGUN_VM_H_
#define IV_LV5_RAILGUN_VM_H_
#include <cstddef>
#include <vector>
#include <string>
#include <iv/notfound.h>
#include <iv/detail/memory.h>
#include <iv/detail/cstdint.h>
#include <iv/detail/tuple.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jserror.h>
#include <iv/lv5/jsregexp.h>
#include <iv/lv5/property.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/chain.h>
#include <iv/lv5/name_iterator.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/vm_fwd.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/operation.h>
#include <iv/lv5/railgun/code.h>
#include <iv/lv5/railgun/frame.h>
#include <iv/lv5/railgun/instruction_fwd.h>
#include <iv/lv5/railgun/jsfunction.h>
#include <iv/lv5/railgun/stack.h>
#include <iv/lv5/railgun/exception.h>
#include <iv/lv5/railgun/direct_threading.h>
#include <iv/lv5/radio/radio.h>

namespace iv {
namespace lv5 {
namespace railgun {

// Global
JSVal VM::Run(Code* code, Error* e) {
  return RunEval(code,
                 ctx_->global_env(),
                 ctx_->global_env(),
                 ctx_->global_obj(), e);
}

// Eval
JSVal VM::RunEval(Code* code,
                  JSEnv* variable_env,
                  JSEnv* lexical_env,
                  JSVal this_binding,
                  Error* e) {
  ScopedArguments args(ctx_, 0, IV_LV5_ERROR(e));
  args.set_this_binding(this_binding);
  Frame* frame = stack_.NewEvalFrame(
      ctx_,
      args.ExtractBase(),
      code,
      variable_env,
      lexical_env);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  frame->InitThisBinding(ctx_, e);
  if (*e) {
    stack_.Unwind(frame);
    return JSEmpty;
  }
  const std::pair<JSVal, State> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res.first;
}

std::pair<JSVal, VM::State> VM::Execute(Arguments* args,
                                        JSVMFunction* func, Error* e) {
  Frame* frame = stack_.NewCodeFrame(
      ctx_,
      args->ExtractBase(),
      func->code(),
      func->scope(),
      func,
      NULL, args->size() + 1, args->IsConstructorCalled());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, STATE_THROW);
  }
  frame->InitThisBinding(ctx_, e);
  if (*e) {
    stack_.Unwind(frame);
    return std::make_pair(JSEmpty, STATE_THROW);
  }
  const std::pair<JSVal, State> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res;
}

std::pair<JSVal, VM::State> VM::Execute(Frame* start, Error* e) {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
  if (!start) {
    // if start frame is NULL, this pass is getting labels table for
    // direct threading mode
#define V(label, N) &&label,
    static const DirectThreadingDispatchTable kDispatchTable = { {
      IV_LV5_RAILGUN_OP_LIST(V)
      NULL
    } };
    // get direct threading dispatch table
    direct_threading_dispatch_table_ = &kDispatchTable;
    return std::make_pair(JSEmpty, STATE_NORMAL);
#undef V
  }
#endif
  assert(start);
  // current frame values
  Frame* frame = start;
  Instruction* instr = frame->data();
  JSVal* register_offset = frame->RegisterFile();
  bool strict = frame->code()->strict();

#define INCREMENT_NEXT(op) (instr += OPLength<OP::op>::value)

#ifdef IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE
// direct threading mode

#define DEFINE_OPCODE(op)\
  case OP::op:\
  op:

#define DISPATCH_ERROR() break

#define DISPATCH(op)\
  do {\
    INCREMENT_NEXT(op);\
    goto *instr->label;\
  } while (0)

#define DISPATCH_WITH_NO_INCREMENT()\
  do {\
    goto *instr->label;\
  } while (0)

#define PREDICT(now, next)\
  do {\
    INCREMENT_NEXT(now);\
    assert(instr->label == &&next);\
    goto next;\
  } while (0)

#define GO_MAIN_LOOP() DISPATCH_WITH_NO_INCREMENT()

#else
// not direct threading mode

#define DEFINE_OPCODE(op)\
  case OP::op:

#define DISPATCH_ERROR() break

#define DISPATCH_WITH_NO_INCREMENT()\
  continue

#define DISPATCH(op)\
  {\
    INCREMENT_NEXT(op);\
    continue;\
  }

#define PREDICT(now, next)\
  INCREMENT_NEXT(now);\
  continue

#define GO_MAIN_LOOP() goto MAIN_LOOP_START

#endif

#define ERR\
  e);\
  if (*e) {\
    DISPATCH_ERROR();\
  }\
((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define JUMPTO(x) (instr = frame->data() + (x))
#define JUMPBY(x) (instr += (x))

#define UNWIND_DYNAMIC_ENV(n)\
do {\
  const uint16_t dynamic_env_level_shrink = (n);\
  JSEnv* dynamic_env_target = frame->lexical_env();\
  assert(frame->dynamic_env_level_ >= dynamic_env_level_shrink);\
  for (uint16_t i = dynamic_env_level_shrink, \
       len = frame->dynamic_env_level_; i < len; ++i) {\
    dynamic_env_target = dynamic_env_target->outer();\
  }\
  frame->set_lexical_env(dynamic_env_target);\
  frame->dynamic_env_level_ = dynamic_env_level_shrink;\
} while (0)
#define REG(n) (register_offset[n])

#define GETITEM(target, i) ((*target)[(i)])

  // main loop
  for (;;) {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
    DISPATCH_WITH_NO_INCREMENT();
#else
 MAIN_LOOP_START:
#endif
    // if ok, use DISPATCH.
    // if error, use DISPATCH_ERROR.
    switch (instr->u32) {
      DEFINE_OPCODE(NOP) {
        // opcode
        DISPATCH(NOP);
      }

      DEFINE_OPCODE(MV) {
        // opcode | dst | src
        REG(instr[1].i32) = REG(instr[2].i32);
        DISPATCH(MV);
      }

      DEFINE_OPCODE(STOP_CODE) {
        // opcode
        const JSVal ret =
            (frame->constructor_call_) ? frame->GetThis() : JSUndefined;

        // no return at last
        // return undefined
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          return std::make_pair(
              ret,
              (frame->constructor_call_) ? STATE_CONSTRUCT : STATE_NORMAL);
        }
        // this code is invoked by JS code
        // EVAL / CALL / CONSTRUCT
        instr = frame->prev_pc_ + OPLength<OP::CALL>::value;
        const int32_t r = frame->r_;
        frame = stack_.Unwind(frame);
        register_offset = frame->RegisterFile();
        REG(r) = ret;
        strict = frame->code()->strict();
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(BUILD_ENV) {
        // opcode | size
        frame->variable_env_ = frame->lexical_env_ =
            JSDeclEnv::New(ctx_, frame->lexical_env(), instr[1].u32);
        DISPATCH(BUILD_ENV);
      }

      DEFINE_OPCODE(INSTANTIATE_DECLARATION_BINDING) {
        // opcode | name | configurable
        JSEnv* const env = frame->variable_env();
        const Symbol name = frame->GetName(instr[1].u32);
        const bool configurable = instr[2].u32;
        if (!env->HasBinding(ctx_, name)) {
          env->CreateMutableBinding(ctx_, name, configurable, ERR);
        } else if (env == ctx_->global_env()) {
          JSObject* const go = ctx_->global_obj();
          const PropertyDescriptor existing_prop = go->GetProperty(ctx_, name);
          if (existing_prop.IsConfigurable()) {
            go->DefineOwnProperty(
                ctx_,
                name,
                DataDescriptor(
                    JSUndefined,
                    ATTR::W | ATTR::E |
                    ((configurable) ? ATTR::C : ATTR::NONE)),
                true, ERR);
          } else {
            if (existing_prop.IsAccessorDescriptor()) {
              e->Report(Error::Type,
                        "create mutable function binding failed");
              DISPATCH_ERROR();
            }
            const DataDescriptor* const data = existing_prop.AsDataDescriptor();
            if (!data->IsWritable() || !data->IsEnumerable()) {
              e->Report(Error::Type,
                        "create mutable function binding failed");
              DISPATCH_ERROR();
            }
          }
        }
        DISPATCH(INSTANTIATE_DECLARATION_BINDING);
      }

      DEFINE_OPCODE(INSTANTIATE_VARIABLE_BINDING) {
        // opcode | name | configurable
        JSEnv* const env = frame->variable_env();
        const Symbol name = frame->GetName(instr[1].u32);
        const bool configurable = instr[2].u32;
        if (!env->HasBinding(ctx_, name)) {
          env->CreateMutableBinding(ctx_, name, configurable, ERR);
          env->SetMutableBinding(ctx_, name, JSUndefined, strict, ERR);
        }
        DISPATCH(INSTANTIATE_VARIABLE_BINDING);
      }

      DEFINE_OPCODE(INSTANTIATE_HEAP_BINDING) {
        // opcode | name | offset | reg | immutable
        // TODO(Constellation) fix bytecode layout
        const bool immutable = instr[4].u32;
        JSDeclEnv* const decl = static_cast<JSDeclEnv*>(frame->variable_env());
        const Symbol name = frame->GetName(instr[1].u32);
        if (immutable) {
          decl->InstantiateImmutable(name, &REG(instr[3].i32), instr[2].u32);
        } else {
          decl->InstantiateMutable(name, &REG(instr[3].i32), instr[2].u32);
        }
        DISPATCH(INSTANTIATE_HEAP_BINDING);
      }

      DEFINE_OPCODE(INITIALIZE_HEAP_IMMUTABLE) {
        // opcode | src | offset
        JSDeclEnv* const decl = static_cast<JSDeclEnv*>(frame->variable_env());
        const JSVal src = REG(instr[1].i32);
        decl->InitializeImmutable(instr[2].u32, src);
        DISPATCH(INITIALIZE_HEAP_IMMUTABLE);
      }

      DEFINE_OPCODE(LOAD_CALLEE) {
        // opcode | dst
        REG(instr[1].i32) = frame->callee();
        DISPATCH(LOAD_CALLEE);
      }

      DEFINE_OPCODE(LOAD_ARGUMENTS) {
        // opcode | dst
        if (!strict) {
          JSObject* obj = JSNormalArguments::New(
              ctx_, frame->callee().object()->AsCallable(),
              frame->code()->params(),
              frame->arguments_crbegin(),
              frame->arguments_crend(),
              static_cast<JSDeclEnv*>(frame->variable_env()),
              ERR);
          REG(instr[1].i32) = obj;
        } else {
          JSObject* obj = JSStrictArguments::New(
              ctx_, frame->callee().object()->AsCallable(),
              frame->arguments_crbegin(),
              frame->arguments_crend(),
              ERR);
          REG(instr[1].i32) = obj;
        }
        DISPATCH(LOAD_ARGUMENTS);
      }

      DEFINE_OPCODE(LOAD_CONST) {
        // opcode | dst | offset
        REG(instr[1].i32) = frame->GetConstant(instr[2].u32);
        DISPATCH(LOAD_CONST);
      }

      DEFINE_OPCODE(LOAD_NAME) {
        // opcode | dst | index
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.LoadName(frame->lexical_env(), name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(LOAD_NAME);
      }

      DEFINE_OPCODE(LOAD_HEAP) {
        // opcode | dst | index | offset | nest
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.LoadHeap(frame->lexical_env(),
                                name, strict, instr[3].u32, instr[4].u32, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(LOAD_HEAP);
      }

      DEFINE_OPCODE(LOAD_GLOBAL) {
        // opcode | dst | index | nop | nop
        JSGlobal* global = ctx_->global_obj();
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.LoadGlobal(global, instr, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(LOAD_GLOBAL);
      }

      DEFINE_OPCODE(LOAD_ELEMENT) {
        // opcode | dst | base | element
        const JSVal base = REG(instr[2].i32);
        const JSVal element = REG(instr[3].i32);
        const JSVal res =
            operation_.LoadElement(base, element, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(LOAD_ELEMENT);
      }

      DEFINE_OPCODE(LOAD_PROP) {
        // opcode | dst | base | name | nop | nop | nop
        const Symbol name = frame->GetName(instr[3].u32);
        const JSVal base = REG(instr[2].i32);
        const JSVal res =
            operation_.LoadProp<
              OP::LOAD_PROP_OWN,
              OP::LOAD_PROP_PROTO,
              OP::LOAD_PROP_CHAIN,
              OP::LOAD_PROP_GENERIC>(instr, base, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(LOAD_PROP);
      }

      DEFINE_OPCODE(LOAD_PROP_OWN) {
        // opcode | dst | base | name | map | offset | nop
        const JSVal base = REG(instr[2].i32);
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol name = frame->GetName(instr[3].u32);
          if (operation_.GetPrimitiveOwnProperty(base, name, &res)) {
            REG(instr[1].i32) = res;
            DISPATCH(LOAD_PROP_OWN);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        assert(obj);
        if (instr[4].map == obj->map()) {
          // cache hit
          const JSVal res =
              obj->GetSlot(instr[5].u32).Get(ctx_, base, ERR);
          REG(instr[1].i32) = res;
        } else {
          // uncache
          const Symbol name = frame->GetName(instr[3].u32);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res =
              operation_.LoadProp(base, name, strict, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(LOAD_PROP_OWN);
      }

      DEFINE_OPCODE(LOAD_PROP_PROTO) {
        // opcode | dst | base | name | map | map | offset
        const JSVal base = REG(instr[2].i32);
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol name = frame->GetName(instr[3].u32);
          if (operation_.GetPrimitiveOwnProperty(base, name, &res)) {
            REG(instr[1].i32) = res;
            DISPATCH(LOAD_PROP_PROTO);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        JSObject* proto = obj->prototype();
        if (instr[4].map == obj->map() &&
            proto && instr[5].map == proto->map()) {
          // cache hit
          const JSVal res =
              proto->GetSlot(instr[6].u32).Get(ctx_, base, ERR);
          REG(instr[1].i32) = res;
        } else {
          // uncache
          const Symbol name = frame->GetName(instr[3].u32);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res =
              operation_.LoadProp(base, name, strict, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(LOAD_PROP_PROTO);
      }

      DEFINE_OPCODE(LOAD_PROP_CHAIN) {
        // opcode | dst | base | name | chain | map | offset
        const JSVal base = REG(instr[2].i32);
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol name = frame->GetName(instr[3].u32);
          if (operation_.GetPrimitiveOwnProperty(base, name, &res)) {
            REG(instr[1].i32) = res;
            DISPATCH(LOAD_PROP_CHAIN);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        if (JSObject* cached = instr[4].chain->Validate(obj, instr[5].map)) {
          // cache hit
          const JSVal res =
              cached->GetSlot(instr[6].u32).Get(ctx_, base, ERR);
          REG(instr[1].i32) = res;
        } else {
          // uncache
          const Symbol name = frame->GetName(instr[3].u32);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res = operation_.LoadProp(base, name, strict, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(LOAD_PROP_CHAIN);
      }

      DEFINE_OPCODE(LOAD_PROP_GENERIC) {
        // opcode | dst | base | name | nop | nop | nop
        const JSVal base = REG(instr[2].i32);
        const Symbol name = frame->GetName(instr[3].u32);
        const JSVal res = operation_.LoadProp(base, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(LOAD_PROP_GENERIC);
      }

      DEFINE_OPCODE(STORE_NAME) {
        // opcode | src | name
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal src = REG(instr[1].i32);
        operation_.StoreName(frame->lexical_env(), name, src, strict, ERR);
        DISPATCH(STORE_NAME);
      }

      DEFINE_OPCODE(STORE_HEAP) {
        // opcode | src | name | offset | nest
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal src = REG(instr[1].i32);
        operation_.StoreHeap(frame->lexical_env(),
                             name, src, strict,
                             instr[3].u32, instr[4].u32, ERR);
        DISPATCH(STORE_HEAP);
      }

      DEFINE_OPCODE(STORE_GLOBAL) {
        // opcode | src | name | nop | nop
        const JSVal src = REG(instr[1].i32);
        JSGlobal* global = ctx_->global_obj();
        if (instr[3].map == global->map()) {
          // map is cached, so use previous index code
          global->PutToSlotOffset(ctx_, instr[4].u32, src, strict, ERR);
        } else {
          const Symbol name = frame->GetName(instr[2].u32);
          Slot slot;
          if (global->GetOwnPropertySlot(ctx_, name, &slot)) {
            instr[3].map = global->map();
            instr[4].u32 = slot.offset();
            global->PutToSlotOffset(ctx_, instr[4].u32, src, strict, ERR);
          } else {
            instr[3].map = NULL;
            operation_.StoreName(ctx_->global_env(), name, src, strict, ERR);
          }
        }
        DISPATCH(STORE_GLOBAL);
      }

      DEFINE_OPCODE(STORE_ELEMENT) {
        // opcode | base | element | src
        const JSVal base = REG(instr[1].i32);
        const JSVal element = REG(instr[2].i32);
        const JSVal src = REG(instr[3].i32);
        operation_.StoreElement(base, element, src, strict, ERR);
        DISPATCH(STORE_ELEMENT);
      }

      DEFINE_OPCODE(STORE_PROP) {
        // opcode | base | index | src | nop | nop
        const JSVal base = REG(instr[1].i32);
        const Symbol name= frame->GetName(instr[2].u32);
        const JSVal src = REG(instr[3].i32);
        operation_.StoreProp(
            base, instr, OP::STORE_PROP_GENERIC, name, src, strict, ERR);
        DISPATCH(STORE_PROP);
      }

      DEFINE_OPCODE(STORE_PROP_GENERIC) {
        // opcode | base | index | src | nop | nop
        const JSVal base = REG(instr[1].i32);
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal src = REG(instr[3].i32);
        operation_.StoreProp(base, name, src, strict, ERR);
        DISPATCH(STORE_PROP_GENERIC);
      }

      DEFINE_OPCODE(DELETE_NAME) {
        // opcode | dst | name
        const Symbol name = frame->GetName(instr[2].u32);
        if (JSEnv* current = operation_.GetEnv(frame->lexical_env(), name)) {
          REG(instr[1].i32) = JSVal::Bool(current->DeleteBinding(ctx_, name));
        } else {
          // not found -> unresolvable reference
          REG(instr[1].i32) = JSTrue;
        }
        DISPATCH(DELETE_NAME);
      }

      DEFINE_OPCODE(DELETE_HEAP) {
        // opcode | dst | name | offset | nest
        REG(instr[1].i32) = JSFalse;
        DISPATCH(DELETE_HEAP);
      }

      DEFINE_OPCODE(DELETE_GLOBAL) {
        // opcode | dst | name | nop | nop
        const Symbol name = frame->GetName(instr[2].u32);
        if (ctx_->global_env()->HasBinding(ctx_, name)) {
          const bool res = ctx_->global_env()->DeleteBinding(ctx_, name);
          REG(instr[1].i32) = JSVal::Bool(res);
        } else {
          // not found -> unresolvable reference
          REG(instr[1].i32) = JSTrue;
        }
        DISPATCH(DELETE_GLOBAL);
      }

      DEFINE_OPCODE(DELETE_ELEMENT) {
        // opcode | dst | base | element
        const JSVal base = REG(instr[2].i32);
        const JSVal element = REG(instr[3].i32);
        base.CheckObjectCoercible(ERR);
        uint32_t index;
        if (element.GetUInt32(&index)) {
          JSObject* const obj = base.ToObject(ctx_, ERR);
          REG(instr[1].i32) = obj;
          const bool result =
              obj->Delete(ctx_,
                          symbol::MakeSymbolFromIndex(index), strict, ERR);
          REG(instr[1].i32) = JSVal::Bool(result);
        } else {
          // const radio::Scope scope(ctx_);
          const Symbol name = element.ToSymbol(ctx_, ERR);
          JSObject* const obj = base.ToObject(ctx_, ERR);
          REG(instr[1].i32) = obj;
          const bool result = obj->Delete(ctx_, name, strict, ERR);
          REG(instr[1].i32) = JSVal::Bool(result);
        }
        DISPATCH(DELETE_ELEMENT);
      }

      DEFINE_OPCODE(DELETE_PROP) {
        // opcode | dst | base | name | nop | nop | nop
        const JSVal base = REG(instr[2].i32);
        const Symbol name = frame->GetName(instr[3].u32);
        base.CheckObjectCoercible(ERR);
        JSObject* const obj = base.ToObject(ctx_, ERR);
        REG(instr[1].i32) = obj;
        const bool res = obj->Delete(ctx_, name, strict, ERR);
        REG(instr[1].i32) = JSVal::Bool(res);
        DISPATCH(DELETE_PROP);
      }

      DEFINE_OPCODE(IF_FALSE) {
        // opcode | jmp | cond
        const JSVal v = REG(instr[2].i32);
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_FALSE);
      }

      DEFINE_OPCODE(IF_TRUE) {
        // opcode | jmp | cond
        const JSVal v = REG(instr[2].i32);
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_TRUE);
      }

      DEFINE_OPCODE(JUMP_SUBROUTINE) {
        // opcode | jmp | addr | flag
        REG(instr[2].i32) =
            ((std::distance(frame->data(), instr) +
              OPLength<OP::JUMP_SUBROUTINE>::value));
        REG(instr[3].i32) = JSVal::Int32(kJumpFromSubroutine);
        JUMPBY(instr[1].diff);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_BY) {
        // opcode | jmp
        JUMPBY(instr[1].diff);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(LOAD_EMPTY) {
        // opcode | dst
        REG(instr[1].i32) = JSEmpty;
        DISPATCH(LOAD_EMPTY);
      }

      DEFINE_OPCODE(LOAD_NULL) {
        // opcode | dst
        REG(instr[1].i32) = JSNull;
        DISPATCH(LOAD_NULL);
      }

      DEFINE_OPCODE(LOAD_TRUE) {
        // opcode | dst
        REG(instr[1].i32) = JSTrue;
        DISPATCH(LOAD_TRUE);
      }

      DEFINE_OPCODE(LOAD_FALSE) {
        // opcode | dst
        REG(instr[1].i32) = JSFalse;
        DISPATCH(LOAD_FALSE);
      }

      DEFINE_OPCODE(LOAD_UNDEFINED) {
        // opcode | dst
        REG(instr[1].i32) = JSUndefined;
        DISPATCH(LOAD_UNDEFINED);
      }

      DEFINE_OPCODE(UNARY_POSITIVE) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        if (src.IsNumber()) {
          REG(instr[1].i32) = src;
        } else {
          const double x = src.ToNumber(ctx_, ERR);
          REG(instr[1].i32) = x;
        }
        DISPATCH(UNARY_POSITIVE);
      }

      DEFINE_OPCODE(UNARY_NEGATIVE) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        const double x = src.ToNumber(ctx_, ERR);
        REG(instr[1].i32) = (-x);
        DISPATCH(UNARY_NEGATIVE);
      }

      DEFINE_OPCODE(UNARY_NOT) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        const bool x = src.ToBoolean(ERR);
        REG(instr[1].i32) = JSVal::Bool(!x);
        DISPATCH(UNARY_NOT);
      }

      DEFINE_OPCODE(UNARY_BIT_NOT) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        if (src.IsInt32()) {
          REG(instr[1].i32) = JSVal::Int32(~src.int32());
        } else {
          const double value = src.ToNumber(ctx_, ERR);
          REG(instr[1].i32) = JSVal::Int32(~core::DoubleToInt32(value));
        }
        DISPATCH(UNARY_BIT_NOT);
      }

      DEFINE_OPCODE(TYPEOF) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        REG(instr[1].i32) = src.TypeOf(ctx_);
        DISPATCH(TYPEOF);
      }

      DEFINE_OPCODE(TYPEOF_NAME) {
        // opcode | dst | name
        const Symbol name = frame->GetName(instr[2].u32);
        if (JSEnv* current = operation_.GetEnv(frame->lexical_env(), name)) {
          const JSVal res = current->GetBindingValue(ctx_, name, strict, ERR);
          REG(instr[1].i32) = res;
          REG(instr[1].i32) = res.TypeOf(ctx_);
        } else {
          // unresolvable reference
          REG(instr[1].i32) = ctx_->global_data()->string_undefined();
        }
        DISPATCH(TYPEOF_NAME);
      }

      DEFINE_OPCODE(TYPEOF_HEAP) {
        // opcode | dst | name | offset | nest
        JSDeclEnv* decl =
            operation_.GetHeapEnv(frame->lexical_env(), instr[4].u32);
        assert(decl);
        const JSVal res = decl->GetByOffset(instr[3].u32, strict, ERR);
        REG(instr[1].i32) = res;
        REG(instr[1].i32) = res.TypeOf(ctx_);
        DISPATCH(TYPEOF_HEAP);
      }

      DEFINE_OPCODE(TYPEOF_GLOBAL) {
        // opcode | dst | name | nop | nop
        const Symbol name = frame->GetName(instr[2].u32);
        if (ctx_->global_env()->HasBinding(ctx_, name)) {
          const JSVal res =
              ctx_->global_env()->GetBindingValue(ctx_, name, strict, ERR);
          REG(instr[1].i32) = res;
          REG(instr[1].i32) = res.TypeOf(ctx_);
        } else {
          // unresolvable reference
          REG(instr[1].i32) = ctx_->global_data()->string_undefined();
        }
        DISPATCH(TYPEOF_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT) {
        // opcode | src
        const JSVal src = REG(instr[1].i32);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<-1>(src.int32())) {
          REG(instr[1].i32) = src.int32() - 1;
        } else {
          const double res = src.ToNumber(ctx_, ERR);
          REG(instr[1].i32) = res - 1;
        }
        DISPATCH(DECREMENT);
      }

      DEFINE_OPCODE(INCREMENT) {
        // opcode | src
        const JSVal src = REG(instr[1].i32);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<+1>(src.int32())) {
          REG(instr[1].i32) = src.int32() + 1;
        } else {
          const double res = src.ToNumber(ctx_, ERR);
          REG(instr[1].i32) = res + 1;
        }
        DISPATCH(INCREMENT);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<-1>(src.int32())) {
          REG(instr[1].i32) = src;
          REG(instr[2].i32) = src.int32() - 1;
        } else {
          const double res = src.ToNumber(ctx_, ERR);
          REG(instr[1].i32) = res;
          REG(instr[2].i32) = res - 1;
        }
        DISPATCH(POSTFIX_DECREMENT);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT) {
        // opcode | dst | src
        const JSVal src = REG(instr[2].i32);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<+1>(src.int32())) {
          REG(instr[1].i32) = src;
          REG(instr[2].i32) = src.int32() + 1;
        } else {
          const double res = src.ToNumber(ctx_, ERR);
          REG(instr[1].i32) = res;
          REG(instr[2].i32) = res + 1;
        }
        DISPATCH(POSTFIX_INCREMENT);
      }

      DEFINE_OPCODE(DECREMENT_NAME) {
        // opcode | dst | name
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementName<-1, 1>(frame->lexical_env(),
                                            name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(DECREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_NAME) {
        // opcode | dst | name
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementName<-1, 0>(frame->lexical_env(),
                                            name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_DECREMENT_NAME);
      }

      DEFINE_OPCODE(INCREMENT_NAME) {
        // opcode | dst | name
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementName<1, 1>(frame->lexical_env(),
                                           name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(INCREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_NAME) {
        // opcode | dst | name
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementName<1, 0>(frame->lexical_env(),
                                           name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_INCREMENT_NAME);
      }

      DEFINE_OPCODE(DECREMENT_HEAP) {
        // opcode | dst | name | offset | nest
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementHeap<-1, 1>(
                frame->lexical_env(), name, strict,
                instr[3].u32, instr[4].u32, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(DECREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_HEAP) {
        // opcode | dst | name | offset | nest
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementHeap<-1, 0>(
                frame->lexical_env(), name, strict,
                instr[3].u32, instr[4].u32, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_DECREMENT_HEAP);
      }

      DEFINE_OPCODE(INCREMENT_HEAP) {
        // opcode | dst | name | offset | nest
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementHeap<1, 1>(
                frame->lexical_env(), name, strict,
                instr[3].u32, instr[4].u32, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(INCREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_HEAP) {
        // opcode | dst | name | offset | nest
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementHeap<1, 0>(
                frame->lexical_env(), name, strict,
                instr[3].u32, instr[4].u32, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_INCREMENT_HEAP);
      }

      DEFINE_OPCODE(DECREMENT_GLOBAL) {
        // opcode | dst | name | nop | nop
        JSGlobal* global = ctx_->global_obj();
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementGlobal<-1, 1>(global, instr, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(DECREMENT_GLOBAL);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_GLOBAL) {
        // opcode | dst | name | nop | nop
        JSGlobal* global = ctx_->global_obj();
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementGlobal<-1, 0>(global, instr, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_DECREMENT_GLOBAL);
      }

      DEFINE_OPCODE(INCREMENT_GLOBAL) {
        // opcode | dst | name | nop | nop
        JSGlobal* global = ctx_->global_obj();
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementGlobal<1, 1>(global, instr, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(INCREMENT_GLOBAL);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_GLOBAL) {
        // opcode | dst | name | nop | nop
        JSGlobal* global = ctx_->global_obj();
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal res =
            operation_.IncrementGlobal<1, 0>(global, instr, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_INCREMENT_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT_ELEMENT) {
        // opcode | dst | base | element
        const JSVal base = REG(instr[2].i32);
        const JSVal element = REG(instr[3].i32);
        const JSVal res =
            operation_.IncrementElement<-1, 1>(base, element, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_ELEMENT) {
        // opcode | dst | base | element
        const JSVal base = REG(instr[2].i32);
        const JSVal element = REG(instr[3].i32);
        const JSVal res =
            operation_.IncrementElement<-1, 0>(base, element, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(INCREMENT_ELEMENT) {
        // opcode | dst | base | element
        const JSVal base = REG(instr[2].i32);
        const JSVal element = REG(instr[3].i32);
        const JSVal res =
            operation_.IncrementElement<1, 1>(base, element, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_ELEMENT) {
        // opcode | dst | base | element
        const JSVal base = REG(instr[2].i32);
        const JSVal element = REG(instr[3].i32);
        const JSVal res =
            operation_.IncrementElement<1, 0>(base, element, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(DECREMENT_PROP) {
        // opcode | dst | base | name | nop | nop | nop
        const JSVal base = REG(instr[2].i32);
        const Symbol name = frame->GetName(instr[3].u32);
        const JSVal res =
            operation_.IncrementProp<-1, 1>(base, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(DECREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_PROP) {
        // opcode | dst | base | name | nop | nop | nop
        const JSVal base = REG(instr[2].i32);
        const Symbol name = frame->GetName(instr[3].u32);
        const JSVal res =
            operation_.IncrementProp<-1, 0>(base, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_DECREMENT_PROP);
      }

      DEFINE_OPCODE(INCREMENT_PROP) {
        // opcode | dst | base | name | nop | nop | nop
        const JSVal base = REG(instr[2].i32);
        const Symbol name = frame->GetName(instr[3].u32);
        const JSVal res =
            operation_.IncrementProp<1, 1>(base, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(INCREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_PROP) {
        // opcode | dst | base | name | nop | nop | nop
        const JSVal base = REG(instr[2].i32);
        const Symbol name = frame->GetName(instr[3].u32);
        const JSVal res =
            operation_.IncrementProp<1, 0>(base, name, strict, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(POSTFIX_INCREMENT_PROP);
      }

      DEFINE_OPCODE(BINARY_ADD) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t sum;
          if (!core::IsAdditionOverflow(lhs.int32(), rhs.int32(), &sum)) {
            REG(instr[1].i32) = JSVal::Int32(sum);
          } else {
            REG(instr[1].i32) =
                JSVal(static_cast<double>(lhs.int32()) +
                      static_cast<double>(rhs.int32()));
          }
        } else {
          const JSVal res = operation_.BinaryAdd(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_ADD);
      }

      DEFINE_OPCODE(BINARY_SUBTRACT) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t dif;
          if (!core::IsSubtractOverflow(lhs.int32(), rhs.int32(), &dif)) {
            REG(instr[1].i32) = JSVal::Int32(dif);
          } else {
            REG(instr[1].i32) =
                JSVal(static_cast<double>(lhs.int32()) -
                      static_cast<double>(rhs.int32()));
          }
        } else {
          const JSVal res = operation_.BinarySub(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_SUBTRACT);
      }

      DEFINE_OPCODE(BINARY_MULTIPLY) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        // if 1 bit is not found in MSB to MSB - 17 bits,
        // this multiply is safe for overflow
        if (lhs.IsInt32() && rhs.IsInt32() &&
            !((lhs.int32() | rhs.int32()) >> 15)) {
          REG(instr[1].i32) = JSVal::Int32(lhs.int32() * rhs.int32());
        } else {
          const JSVal res = operation_.BinaryMultiply(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_MULTIPLY);
      }

      DEFINE_OPCODE(BINARY_DIVIDE) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        const JSVal res = operation_.BinaryDivide(lhs, rhs, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(BINARY_DIVIDE);
      }

      DEFINE_OPCODE(BINARY_MODULO) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        // check rhs is more than 0 (n % 0 == NaN)
        // lhs is >= 0 and rhs is > 0 because example like
        //   -1 % -1
        // should return -0.0, so this value is double
        if (lhs.IsInt32() && rhs.IsInt32() &&
            lhs.int32() >= 0 && rhs.int32() > 0) {
          REG(instr[1].i32) = JSVal::Int32(lhs.int32() % rhs.int32());
        } else {
          const JSVal res = operation_.BinaryModulo(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_MODULO);
      }

      DEFINE_OPCODE(BINARY_LSHIFT) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) =
              JSVal::Int32(lhs.int32() << (rhs.int32() & 0x1f));
        } else {
          const JSVal res = operation_.BinaryLShift(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_LSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) =
              JSVal::Int32(lhs.int32() >> (rhs.int32() & 0x1f));
        } else {
          const JSVal res = operation_.BinaryRShift(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_RSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT_LOGICAL) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        uint32_t left_result;
        if (lhs.GetUInt32(&left_result) && rhs.IsInt32()) {
          REG(instr[1].i32) =
              JSVal::UInt32(left_result >> (rhs.int32() & 0x1f));
        } else {
          const JSVal res = operation_.BinaryRShiftLogical(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_RSHIFT_LOGICAL);
      }

      DEFINE_OPCODE(BINARY_LT) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() < rhs.int32());
        } else {
          const JSVal res = operation_.BinaryCompareLT(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_LT);
      }

      DEFINE_OPCODE(BINARY_LTE) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() <= rhs.int32());
        } else {
          const JSVal res = operation_.BinaryCompareLTE(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_LTE);
      }

      DEFINE_OPCODE(BINARY_GT) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() > rhs.int32());
        } else {
          const JSVal res = operation_.BinaryCompareGT(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_GT);
      }

      DEFINE_OPCODE(BINARY_GTE) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() >= rhs.int32());
        } else {
          const JSVal res = operation_.BinaryCompareGTE(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_GTE);
      }

      DEFINE_OPCODE(BINARY_INSTANCEOF) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        const JSVal res = operation_.BinaryInstanceof(lhs, rhs, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(BINARY_INSTANCEOF);
      }

      DEFINE_OPCODE(BINARY_IN) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        const JSVal res = operation_.BinaryIn(lhs, rhs, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(BINARY_IN);
      }

      DEFINE_OPCODE(BINARY_EQ) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() == rhs.int32());
        } else {
          const JSVal res = operation_.BinaryEqual(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_EQ);
      }

      DEFINE_OPCODE(BINARY_STRICT_EQ) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() == rhs.int32());
        } else {
          const JSVal res = operation_.BinaryStrictEqual(lhs, rhs);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_STRICT_EQ);
      }

      DEFINE_OPCODE(BINARY_NE) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() != rhs.int32());
        } else {
          const JSVal res = operation_.BinaryNotEqual(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_NE);
      }

      DEFINE_OPCODE(BINARY_STRICT_NE) {
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Bool(lhs.int32() != rhs.int32());
        } else {
          const JSVal res = operation_.BinaryStrictNotEqual(lhs, rhs);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_STRICT_NE);
      }

      DEFINE_OPCODE(BINARY_BIT_AND) {  // &
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Int32(lhs.int32() & rhs.int32());
        } else {
          const JSVal res = operation_.BinaryBitAnd(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_BIT_AND);
      }

      DEFINE_OPCODE(BINARY_BIT_XOR) {  // ^
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Int32(lhs.int32() ^ rhs.int32());
        } else {
          const JSVal res = operation_.BinaryBitXor(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_BIT_XOR);
      }

      DEFINE_OPCODE(BINARY_BIT_OR) {  // |
        // opcode | dst | lhs | rhs
        const JSVal lhs = REG(instr[2].i32);
        const JSVal rhs = REG(instr[3].i32);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i32) = JSVal::Int32(lhs.int32() | rhs.int32());
        } else {
          const JSVal res = operation_.BinaryBitOr(lhs, rhs, ERR);
          REG(instr[1].i32) = res;
        }
        DISPATCH(BINARY_BIT_OR);
      }

      DEFINE_OPCODE(RETURN) {
        // opcode | src
        const JSVal src = REG(instr[1].i32);
        const JSVal ret =
            (frame->constructor_call_ && !src.IsObject()) ?
            frame->GetThis() : src;
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          return std::make_pair(ret, STATE_RETURN);
        }
        // this code is invoked by JS code
        // EVAL / CALL / CONSTRUCT
        instr = frame->prev_pc_ + OPLength<OP::CALL>::value;
        const int32_t r = frame->r_;
        frame = stack_.Unwind(frame);
        register_offset = frame->RegisterFile();
        REG(r) = ret;
        strict = frame->code()->strict();
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(RETURN_SUBROUTINE) {
        // opcode | jmp | flag
        const JSVal flag = REG(instr[2].i32);
        const JSVal v = REG(instr[1].i32);
        assert(flag.IsInt32());
        const int32_t f = flag.int32();
        if (f == kJumpFromSubroutine) {
          // JUMP_SUBROUTINE
          // return to caller
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          DISPATCH_WITH_NO_INCREMENT();
        } else {
          // ERROR FINALLY JUMP
          // rethrow error
          assert(f == kJumpFromFinally);
          e->Report(v);
          DISPATCH_ERROR();
        }
      }

      DEFINE_OPCODE(FORIN_SETUP) {
        // opcode | jmp | iterator | enumerable
        // TODO(Constellation): fix this for Exact GC
        const JSVal enumerable = REG(instr[3].i32);
        if (enumerable.IsNullOrUndefined()) {
          JUMPBY(instr[1].diff);  // skip for-in stmt
          DISPATCH_WITH_NO_INCREMENT();
        }
        JSObject* const obj = enumerable.ToObject(ctx_, ERR);
        NameIterator* it = NameIterator::New(ctx_, obj);
        REG(instr[2].i32) = JSVal::Cell(it);
        PREDICT(FORIN_SETUP, FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(FORIN_ENUMERATE) {
        // opcode | jmp | dst | iterator
        NameIterator* it =
            reinterpret_cast<NameIterator*>(REG(instr[3].i32).cell());
        if (it->Has()) {
          const Symbol sym = it->Get();
          it->Next();
          REG(instr[2].i32) = JSString::New(ctx_, sym);
        } else {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(THROW) {
        // opcode | src
        e->Report(REG(instr[1].i32));
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(RAISE_IMMUTABLE) {
        // opcode | name
        const Symbol name = frame->GetName(instr[1].u32);
        operation_.RaiseImmutable(name, e);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(RAISE_REFERENCE) {
        // opcode
        operation_.RaiseReferenceError(e);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(TO_NUMBER_AND_RAISE_REFERENCE) {
        // opcode | src
        const JSVal src = REG(instr[1].i32);
        src.ToNumber(ctx_, ERR);
        operation_.RaiseReferenceError(e);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(DEBUGGER) {
        // opcode
        DISPATCH(DEBUGGER);
      }

      DEFINE_OPCODE(WITH_SETUP) {
        // opcode | src
        // TODO(Constellation): fix this for Exact GC
        const JSVal src = REG(instr[1].i32);
        JSObject* const obj = src.ToObject(ctx_, ERR);
        JSObjectEnv* const with_env =
            JSObjectEnv::New(ctx_, frame->lexical_env(), obj);
        with_env->set_provide_this(true);
        frame->set_lexical_env(with_env);
        frame->dynamic_env_level_ += 1;
        DISPATCH(WITH_SETUP);
      }

      DEFINE_OPCODE(POP_ENV) {
        // opcode
        frame->set_lexical_env(frame->lexical_env()->outer());
        frame->dynamic_env_level_ -= 1;
        DISPATCH(POP_ENV);
      }

      DEFINE_OPCODE(TRY_CATCH_SETUP) {
        // opcode | error | name
        const Symbol name = frame->GetName(instr[2].u32);
        const JSVal error = REG(instr[1].i32);
        JSEnv* const catch_env =
            JSStaticEnv::New(ctx_, frame->lexical_env(), name, error);
        frame->set_lexical_env(catch_env);
        frame->dynamic_env_level_ += 1;
        DISPATCH(TRY_CATCH_SETUP);
      }

      DEFINE_OPCODE(LOAD_ARRAY) {
        // opcode | dst | size
        REG(instr[1].i32) = JSArray::ReservedNew(ctx_, instr[2].u32);
        DISPATCH(LOAD_ARRAY);
      }

      DEFINE_OPCODE(INIT_VECTOR_ARRAY_ELEMENT) {
        // opcode | ary | reg | index | size
        JSArray* ary = static_cast<JSArray*>(REG(instr[1].i32).object());
        const JSVal* reg = &REG(instr[2].i32);
        ary->SetToVector(instr[3].u32, reg, reg + instr[4].u32);
        DISPATCH(INIT_VECTOR_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(INIT_SPARSE_ARRAY_ELEMENT) {
        // opcode | ary | reg | index | size
        JSArray* ary = static_cast<JSArray*>(REG(instr[1].i32).object());
        const JSVal* reg = &REG(instr[2].i32);
        ary->SetToMap(instr[3].u32, reg, reg + instr[4].u32);
        DISPATCH(INIT_SPARSE_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(LOAD_OBJECT) {
        // opcode | dst | map
        REG(instr[1].i32) = JSObject::New(ctx_, instr[2].map);
        DISPATCH(LOAD_OBJECT);
      }

      DEFINE_OPCODE(LOAD_FUNCTION) {
        // opcode | dst | code
        Code* target = frame->code()->codes()[instr[2].u32];
        REG(instr[1].i32) =
            JSVMFunction::New(ctx_, target, frame->lexical_env());
        DISPATCH(LOAD_FUNCTION);
      }

      DEFINE_OPCODE(LOAD_REGEXP) {
        // opcode | dst | const
        REG(instr[1].i32) = JSRegExp::New(
            ctx_,
            static_cast<JSRegExp*>(
                frame->GetConstant(instr[2].u32).object()));
        DISPATCH(LOAD_REGEXP);
      }

      DEFINE_OPCODE(STORE_OBJECT_DATA) {
        // opcode | obj | item | offset | merged
        assert(REG(instr[1].i32).IsObject());
        JSObject* const obj = REG(instr[1].i32).object();
        const JSVal value = REG(instr[2].i32);
        if (instr[4].u32) {
          obj->GetSlot(instr[3].u32) =
              PropertyDescriptor::Merge(
                  DataDescriptor(value, ATTR::W | ATTR::E | ATTR::C),
              obj->GetSlot(instr[3].u32));
        } else {
          obj->GetSlot(instr[3].u32) =
              DataDescriptor(value, ATTR::W | ATTR::E | ATTR::C);
        }
        assert(!*e);
        DISPATCH(STORE_OBJECT_DATA);
      }

      DEFINE_OPCODE(STORE_OBJECT_GET) {
        // opcode | obj | item | offset | merged
        JSObject* const obj = REG(instr[1].i32).object();
        const JSVal value = REG(instr[2].i32);
        if (instr[4].u32) {
          obj->GetSlot(instr[3].u32) =
              PropertyDescriptor::Merge(
                  AccessorDescriptor(value.object(), NULL,
                                     ATTR::E | ATTR::C | ATTR::UNDEF_SETTER),
              obj->GetSlot(instr[3].u32));
        } else {
          obj->GetSlot(instr[3].u32) =
              AccessorDescriptor(value.object(), NULL,
                                 ATTR::E | ATTR::C | ATTR::UNDEF_SETTER);
        }
        assert(!*e);
        DISPATCH(STORE_OBJECT_GET);
      }

      DEFINE_OPCODE(STORE_OBJECT_SET) {
        // opcode | obj | item | offset | merged
        JSObject* const obj = REG(instr[1].i32).object();
        const JSVal value = REG(instr[2].i32);
        if (instr[4].u32) {
          obj->GetSlot(instr[3].u32) =
              PropertyDescriptor::Merge(
                  AccessorDescriptor(NULL, value.object(),
                                     ATTR::E | ATTR::C | ATTR::UNDEF_GETTER),
              obj->GetSlot(instr[3].u32));
        } else {
          obj->GetSlot(instr[3].u32) =
              AccessorDescriptor(NULL, value.object(),
                                 ATTR::E | ATTR::C | ATTR::UNDEF_GETTER);
        }
        assert(!*e);
        DISPATCH(STORE_OBJECT_SET);
      }

      DEFINE_OPCODE(CALL) {
        // opcode | dst | callee | offset | argc_with_this
        const JSVal callee = REG(instr[2].i32);
        JSVal* offset = &REG(instr[3].i32);
        const int argc_with_this = static_cast<int>(instr[4].u32);
        if (!callee.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func = callee.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          Code* code = vm_func->code();
          if (code->empty()) {
            REG(instr[1].i32) = JSUndefined;
            DISPATCH(CALL);
          }
          Frame* new_frame = stack_.NewCodeFrame(
              ctx_,
              offset,
              code,
              vm_func->scope(),
              func,
              instr, argc_with_this, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          register_offset = frame->RegisterFile();
          strict = frame->code()->strict();
          frame->InitThisBinding(ctx_, ERR);
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        const JSVal res =
            operation_.Invoke(func, offset, argc_with_this, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(CALL);
      }

      DEFINE_OPCODE(CONSTRUCT) {
        // opcode | dst | callee | offset | argc_with_this
        const JSVal callee = REG(instr[2].i32);
        JSVal* offset = &REG(instr[3].i32);
        const int argc_with_this = static_cast<int>(instr[4].u32);
        if (!callee.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func = callee.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          Code* code = vm_func->code();
          Frame* new_frame = stack_.NewCodeFrame(
              ctx_,
              offset,
              code,
              vm_func->scope(),
              func,
              instr, argc_with_this, true);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          register_offset = frame->RegisterFile();
          strict = code->strict();
          JSObject* const obj = JSObject::New(ctx_, code->ConstructMap(ctx_));
          frame->set_this_binding(obj);
          const JSVal proto = func->Get(ctx_, symbol::prototype(), ERR);
          if (proto.IsObject()) {
            obj->set_prototype(proto.object());
          }
          frame->InitThisBinding(ctx_, ERR);
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        const JSVal res =
            operation_.Construct(func, offset, argc_with_this, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(CONSTRUCT);
      }

      DEFINE_OPCODE(EVAL) {
        // opcode | dst | callee | offset | argc_with_this
        // maybe direct call to eval
        const JSVal callee = REG(instr[2].i32);
        JSVal* offset = &REG(instr[3].i32);
        const int argc_with_this = static_cast<int>(instr[4].u32);
        if (!callee.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func = callee.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          Code* code = vm_func->code();
          if (code->empty()) {
            REG(instr[1].i32) = JSUndefined;
            DISPATCH(EVAL);
          }
          Frame* new_frame = stack_.NewCodeFrame(
              ctx_,
              offset,
              code,
              vm_func->scope(),
              func,
              instr, argc_with_this, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          register_offset = frame->RegisterFile();
          strict = frame->code()->strict();
          frame->InitThisBinding(ctx_, ERR);
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        const JSVal res =
            operation_.InvokeMaybeEval(func,
                                       offset, argc_with_this, frame, ERR);
        REG(instr[1].i32) = res;
        DISPATCH(EVAL);
      }

      DEFINE_OPCODE(PREPARE_DYNAMIC_CALL) {
        // opcode | dst | basedst | name
        const Symbol name = frame->GetName(instr[3].u32);
        if (JSEnv* target_env = operation_.GetEnv(frame->lexical_env(), name)) {
          const JSVal res = target_env->GetBindingValue(ctx_, name, false, ERR);
          REG(instr[1].i32) = res;
          REG(instr[2].i32) = target_env->ImplicitThisValue();
        } else {
          operation_.RaiseReferenceError(name, e);
          DISPATCH_ERROR();
        }
        DISPATCH(PREPARE_DYNAMIC_CALL);
      }

      default: {
        std::printf("%s\n", OP::String(instr->u32));
        UNREACHABLE();
      }
    }  // switch

    // search exception handler or finally handler.
    // if finally handler found, set value to notify that RETURN_SUBROUTINE
    // should rethrow exception.
    assert(*e);
    while (true) {
      const ExceptionTable& table = frame->code()->exception_table();
      for (ExceptionTable::const_iterator it = table.begin(),
           last = table.end(); it != last; ++it) {
        const Handler& handler = *it;
        const uint32_t offset = static_cast<uint32_t>(instr - frame->data());
        if (offset < handler.begin()) {
          break;  // not found in this exception table
        } else if (offset < handler.end()) {
          assert(handler.begin() <= offset);
          const JSVal error = JSError::Detail(ctx_, e);
          e->Clear();
          UNWIND_DYNAMIC_ENV(handler.dynamic_env_level());
          if (handler.type() == Handler::FINALLY) {
            REG(handler.flag()) = kJumpFromFinally;
            REG(handler.jmp()) = error;
          } else {
            REG(handler.ret()) = error;
          }
          JUMPTO(handler.end());
          GO_MAIN_LOOP();
        }
      }
      // handler not in this frame
      // so, unwind frame and search again
      if (frame->prev_pc_ == NULL) {
        // this code is invoked by native function
        // so, not unwind and return (continues to after for main loop)
        break;
      } else {
        // unwind frame
        instr = frame->prev_pc_;
        frame = stack_.Unwind(frame);
        register_offset = frame->RegisterFile();
        strict = frame->code()->strict();
      }
    }
    break;
  }  // for main loop
  assert(*e);
  return std::make_pair(JSEmpty, STATE_THROW);
#undef INCREMENT_NEXT
#undef DISPATCH_ERROR
#undef DISPATCH
#undef DISPATCH_WITH_NO_INCREMENT
#undef PREDICT
#undef DEFINE_OPCODE
#undef JUMPTO
#undef JUMPBY
#undef UNWIND_DYNAMIC_ENV
#undef GETITEM
#undef REG
}  // NOLINT

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_VM_H_
