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

// Global start
JSVal VM::Run(Code* code, Error* e) {
  Frame* frame = stack_.NewGlobalFrame(ctx_, code);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  InitThisBinding(ctx_, frame, e);
  if (*e) {
    stack_.Unwind(frame);
    return JSEmpty;
  }
  const std::pair<JSVal, State> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res.first;
}

// Global
JSVal VM::RunGlobal(Code* code, Error* e) {
  ScopedArguments args(ctx_, 0, IV_LV5_ERROR(e));
  args.set_this_binding(ctx_->global_obj());
  Frame* frame = stack_.NewEvalFrame(
      ctx_,
      stack_.GetTop(),
      code,
      ctx_->global_env(),
      ctx_->global_env());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  InitThisBinding(ctx_, frame, e);
  if (*e) {
    stack_.Unwind(frame);
    return JSEmpty;
  }
  const std::pair<JSVal, State> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res.first;
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
      stack_.GetTop(),
      code,
      variable_env,
      lexical_env);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  InitThisBinding(ctx_, frame, e);
  if (*e) {
    stack_.Unwind(frame);
    return JSEmpty;
  }
  const std::pair<JSVal, State> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res.first;
}

std::pair<JSVal, VM::State> VM::Execute(const Arguments& args,
                                        JSVMFunction* func, Error* e) {
  Frame* frame = stack_.NewCodeFrame(
      ctx_,
      args.GetEnd(),
      func->code(),
      func->scope(),
      func,
      NULL, args.size(), args.IsConstructorCalled());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, STATE_THROW);
  }
  InitThisBinding(ctx_, frame, e);
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
  JSVal* sp = frame->stacktop();
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

#ifdef DEBUG
#define PUSH(x)\
  do {\
    assert(sp < frame->GetFrameEnd());\
    (*sp++ = (x));\
  } while (0)
#else
#define PUSH(x) (*sp++ = (x))
#endif

#define POP() (*--sp)
#define POP_UNUSED() (--sp)
#define STACKADJ(n) (sp -= (n))
#define UNWIND_STACK(n) (sp = (frame->stacktop() + (n)))
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
#define STACK_DEPTH() (sp - frame->stacktop())
#define TOP() (sp[-1])
#define SECOND() (sp[-2])
#define THIRD() (sp[-3])
#define FOURTH() (sp[-4])
#define PEEK(n) (sp[-(n)])
#define SET_TOP(v) (sp[-1] = (v))
#define SET_SECOND(v) (sp[-2] = (v))
#define SET_THIRD(v) (sp[-3] = (v))
#define SET_FOURTH(v) (sp[-4] = (v))
#define SET_VALUE(n, v) (sp[-(n)] = (v))

#define GETLOCAL(i) (frame->GetLocal()[(i)])
#define SETLOCAL(i, v) do {\
(frame->GetLocal()[(i)]) = (v);\
} while (0)

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
    switch (instr->value) {
      DEFINE_OPCODE(NOP) {
        DISPATCH(NOP);
      }

      DEFINE_OPCODE(STOP_CODE) {
        // no return at last
        // return undefined
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          const JSVal ret =
              (frame->constructor_call_ && !frame->ret_.IsObject()) ?
              frame->GetThis() : frame->ret_;
          return std::make_pair(
              ret,
              (frame->constructor_call_) ? STATE_CONSTRUCT : STATE_NORMAL);
        }
        frame->ret_ = JSUndefined;
        const JSVal ret =
            (frame->constructor_call_ && !frame->ret_.IsObject()) ?
            frame->GetThis() : frame->ret_;
        // this code is invoked by JS code
        instr = frame->prev_pc_ + 2;  // EVAL / CALL / CONSTRUCT => 2
        sp = frame->GetPreviousFrameStackTop();
        frame = stack_.Unwind(frame);
        strict = frame->code()->strict();
        PUSH(ret);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(LOAD_PARAM) {
        PUSH(frame->GetArg(instr[1].value));
        DISPATCH(LOAD_PARAM);
      }

      DEFINE_OPCODE(BUILD_ENV) {
        JSDeclEnv* const env =
            JSDeclEnv::New(ctx_, frame->lexical_env(), instr[1].value);
        frame->variable_env_ = frame->lexical_env_ = env;
        DISPATCH(BUILD_ENV);
      }

      DEFINE_OPCODE(INSTANTIATE_DECLARATION_BINDING) {
        JSEnv* const env = frame->variable_env();
        const Symbol s = frame->GetName(instr[1].value);
        const bool configurable = instr[2].value;
        if (!env->HasBinding(ctx_, s)) {
          env->CreateMutableBinding(ctx_, s, configurable, ERR);
        } else if (env == ctx_->global_env()) {
          JSObject* const go = ctx_->global_obj();
          const PropertyDescriptor existing_prop = go->GetProperty(ctx_, s);
          if (existing_prop.IsConfigurable()) {
            go->DefineOwnProperty(
                ctx_,
                s,
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
        JSEnv* const env = frame->variable_env();
        const Symbol s = frame->GetName(instr[1].value);
        const bool configurable = instr[2].value;
        if (!env->HasBinding(ctx_, s)) {
          env->CreateMutableBinding(ctx_, s, configurable, ERR);
          env->SetMutableBinding(ctx_, s, JSUndefined, strict, ERR);
        }
        DISPATCH(INSTANTIATE_VARIABLE_BINDING);
      }

      DEFINE_OPCODE(INSTANTIATE_HEAP_BINDING) {
        const bool immutable = instr[3].value;
        JSDeclEnv* const decl = static_cast<JSDeclEnv*>(frame->variable_env());
        const Symbol s = frame->GetName(instr[1].value);
        if (immutable) {
          decl->InstantiateImmutable(s, instr[2].value);
        } else {
          decl->InstantiateMutable(s, instr[2].value);
        }
        DISPATCH(INSTANTIATE_HEAP_BINDING);
      }

      DEFINE_OPCODE(INITIALIZE_HEAP_IMMUTABLE) {
        JSDeclEnv* const decl = static_cast<JSDeclEnv*>(frame->variable_env());
        decl->InitializeImmutable(instr[1].value, TOP());
        DISPATCH(INITIALIZE_HEAP_IMMUTABLE);
      }

      DEFINE_OPCODE(LOAD_CALLEE) {
        PUSH(frame->callee());
        DISPATCH(LOAD_CALLEE);
      }

      DEFINE_OPCODE(BUILD_ARGUMENTS) {
        if (!strict) {
          JSObject* args_obj = JSNormalArguments::New(
              ctx_, frame->callee().object()->AsCallable(),
              frame->code()->params(),
              frame->arguments_crbegin(),
              frame->arguments_crend(),
              static_cast<JSDeclEnv*>(frame->variable_env()),
              ERR);
          PUSH(args_obj);
        } else {
          JSObject* args_obj = JSStrictArguments::New(
              ctx_, frame->callee().object()->AsCallable(),
              frame->arguments_crbegin(),
              frame->arguments_crend(),
              ERR);
          PUSH(args_obj);
        }
        DISPATCH(BUILD_ARGUMENTS);
      }

      DEFINE_OPCODE(LOAD_CONST) {
        PUSH(frame->GetConstant(instr[1].value));
        DISPATCH(LOAD_CONST);
      }

      DEFINE_OPCODE(LOAD_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal w =
            operation_.LoadName(frame->lexical_env(), s, strict, ERR);
        PUSH(w);
        DISPATCH(LOAD_NAME);
      }

      DEFINE_OPCODE(LOAD_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal w =
            operation_.LoadHeap(frame->lexical_env(),
                                s, strict, instr[2].value, instr[3].value, ERR);
        PUSH(w);
        DISPATCH(LOAD_HEAP);
      }

      DEFINE_OPCODE(LOAD_LOCAL) {
        PUSH(GETLOCAL(instr[1].value));
        DISPATCH(LOAD_LOCAL);
      }

      DEFINE_OPCODE(LOAD_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal val =
            operation_.LoadGlobal(global, instr, s, strict, ERR);
        PUSH(val);
        DISPATCH(LOAD_GLOBAL);
      }

      DEFINE_OPCODE(LOAD_ELEMENT) {
        const JSVal res = operation_.LoadElement(SECOND(), TOP(), strict, ERR);
        POP_UNUSED();
        SET_TOP(res);
        DISPATCH(LOAD_ELEMENT);
      }

      DEFINE_OPCODE(LOAD_PROP) {
        // opcode | name | nop | nop | nop | nop
        const JSVal base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        TOP() =
            operation_.LoadProp<
              OP::LOAD_PROP_OWN,
              OP::LOAD_PROP_PROTO,
              OP::LOAD_PROP_CHAIN,
              OP::LOAD_PROP_GENERIC>(instr, base, s, strict, ERR);
        DISPATCH(LOAD_PROP);
      }

      DEFINE_OPCODE(LOAD_PROP_OWN) {
        // opcode | name | map | offset | nop | nop
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol s = frame->GetName(instr[1].value);
          if (operation_.GetPrimitiveOwnProperty(base, s, &res)) {
            SET_TOP(res);
            DISPATCH(LOAD_PROP_OWN);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        assert(obj);
        if (instr[2].map == obj->map()) {
          // cache hit
          TOP() = obj->GetSlot(instr[3].value).Get(ctx_, base, ERR);
        } else {
          // uncache
          const Symbol s = frame->GetName(instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          TOP() = operation_.LoadProp(base, s, strict, ERR);
        }
        DISPATCH(LOAD_PROP_OWN);
      }

      DEFINE_OPCODE(LOAD_PROP_PROTO) {
        // opcode | name | map | map | offset | nop
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol s = frame->GetName(instr[1].value);
          if (operation_.GetPrimitiveOwnProperty(base, s, &res)) {
            SET_TOP(res);
            DISPATCH(LOAD_PROP_PROTO);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        JSObject* proto = obj->prototype();
        if (instr[2].map == obj->map() &&
            proto && instr[3].map == proto->map()) {
          // cache hit
          TOP() = proto->GetSlot(instr[4].value).Get(ctx_, base, ERR);
        } else {
          // uncache
          const Symbol s = frame->GetName(instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          TOP() = operation_.LoadProp(base, s, strict, ERR);
        }
        DISPATCH(LOAD_PROP_PROTO);
      }

      DEFINE_OPCODE(LOAD_PROP_CHAIN) {
        // opcode | name | chain | map | offset | nop
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol s = frame->GetName(instr[1].value);
          if (operation_.GetPrimitiveOwnProperty(base, s, &res)) {
            SET_TOP(res);
            DISPATCH(LOAD_PROP_CHAIN);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        if (JSObject* cached = instr[2].chain->Validate(obj, instr[3].map)) {
          // cache hit
          TOP() =
              cached->GetSlot(instr[4].value).Get(ctx_, base, ERR);
        } else {
          // uncache
          const Symbol s = frame->GetName(instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          TOP() = operation_.LoadProp(base, s, strict, ERR);
        }
        DISPATCH(LOAD_PROP_CHAIN);
      }

      DEFINE_OPCODE(LOAD_PROP_GENERIC) {
        // no cache
        const JSVal base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        TOP() = operation_.LoadProp(base, s, strict, ERR);
        DISPATCH(LOAD_PROP_GENERIC);
      }

      DEFINE_OPCODE(STORE_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal v = TOP();
        operation_.StoreName(frame->lexical_env(), s, v, strict, ERR);
        DISPATCH(STORE_NAME);
      }

      DEFINE_OPCODE(STORE_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal v = TOP();
        operation_.StoreHeap(frame->lexical_env(),
                             s, v, strict, instr[2].value, instr[3].value, ERR);
        DISPATCH(STORE_HEAP);
      }

      DEFINE_OPCODE(STORE_LOCAL) {
        const JSVal v = TOP();
        SETLOCAL(instr[1].value, v);
        DISPATCH(STORE_LOCAL);
      }

      DEFINE_OPCODE(STORE_LOCAL_IMMUTABLE) {
        assert(!strict);
        DISPATCH(STORE_LOCAL_IMMUTABLE);
      }

      DEFINE_OPCODE(STORE_GLOBAL) {
        const JSVal v = TOP();
        JSGlobal* global = ctx_->global_obj();
        if (instr[2].map == global->map()) {
          // map is cached, so use previous index code
          global->PutToSlotOffset(ctx_, instr[3].value, v, strict, ERR);
        } else {
          const Symbol s = frame->GetName(instr[1].value);
          Slot slot;
          if (global->GetOwnPropertySlot(ctx_, s, &slot)) {
            instr[2].map = global->map();
            instr[3].value = slot.offset();
            global->PutToSlotOffset(ctx_, instr[3].value, v, strict, ERR);
          } else {
            instr[2].map = NULL;
            operation_.StoreName(ctx_->global_env(), s, v, strict, ERR);
          }
        }
        DISPATCH(STORE_GLOBAL);
      }

      DEFINE_OPCODE(STORE_ELEMENT) {
        const JSVal w = TOP();
        const JSVal element = SECOND();
        const JSVal base = THIRD();
        operation_.StoreElement(base, element, w, strict, ERR);
        THIRD() = w;
        STACKADJ(2);
        DISPATCH(STORE_ELEMENT);
      }

      DEFINE_OPCODE(STORE_PROP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal w = TOP();
        const JSVal base = SECOND();
        operation_.StoreProp(base,
                             instr, OP::STORE_PROP_GENERIC, s, w, strict, ERR);
        SECOND() = w;
        POP_UNUSED();
        DISPATCH(STORE_PROP);
      }

      DEFINE_OPCODE(STORE_PROP_GENERIC) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal w = TOP();
        const JSVal base = SECOND();
        operation_.StoreProp(base, s, w, strict, ERR);
        SECOND() = w;
        POP_UNUSED();
        DISPATCH(STORE_PROP_GENERIC);
      }

      DEFINE_OPCODE(STORE_CALL_RESULT) {
        // lv5 reject `func() = 20`
        e->Report(Error::Reference, "target is not reference");
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(DELETE_CALL_RESULT) {
        // lv5 ignore `delete func()`
        SET_TOP(JSTrue);
        DISPATCH(DELETE_CALL_RESULT);
      }

      DEFINE_OPCODE(DELETE_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        if (JSEnv* current = operation_.GetEnv(frame->lexical_env(), s)) {
          const bool res = current->DeleteBinding(ctx_, s);
          PUSH(JSVal::Bool(res));
        } else {
          // not found -> unresolvable reference
          PUSH(JSTrue);
        }
        DISPATCH(DELETE_NAME);
      }

      DEFINE_OPCODE(DELETE_HEAP) {
        PUSH(JSFalse);
        DISPATCH(DELETE_HEAP);
      }

      DEFINE_OPCODE(DELETE_LOCAL) {
        PUSH(JSFalse);
        DISPATCH(DELETE_LOCAL);
      }

      DEFINE_OPCODE(DELETE_GLOBAL) {
        const Symbol s = frame->GetName(instr[1].value);
        if (ctx_->global_env()->HasBinding(ctx_, s)) {
          const bool res = ctx_->global_env()->DeleteBinding(ctx_, s);
          PUSH(JSVal::Bool(res));
        } else {
          // not found -> unresolvable reference
          PUSH(JSTrue);
        }
        DISPATCH(DELETE_GLOBAL);
      }

      DEFINE_OPCODE(DELETE_ELEMENT) {
        const JSVal element = TOP();
        const JSVal base = SECOND();
        base.CheckObjectCoercible(ERR);
        uint32_t index;
        if (element.GetUInt32(&index)) {
          JSObject* const obj = base.ToObject(ctx_, ERR);
          SET_TOP(obj);
          const bool result =
              obj->Delete(ctx_,
                          symbol::MakeSymbolFromIndex(index), strict, ERR);
          POP_UNUSED();
          SET_TOP(JSVal::Bool(result));
        } else {
          // const radio::Scope scope(ctx_);
          const Symbol s = element.ToSymbol(ctx_, ERR);
          JSObject* const obj = base.ToObject(ctx_, ERR);
          SET_TOP(obj);
          const bool result = obj->Delete(ctx_, s, strict, ERR);
          POP_UNUSED();
          SET_TOP(JSVal::Bool(result));
        }
        DISPATCH(DELETE_ELEMENT);
      }

      DEFINE_OPCODE(DELETE_PROP) {
        const JSVal base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        base.CheckObjectCoercible(ERR);
        JSObject* const obj = base.ToObject(ctx_, ERR);
        SET_TOP(obj);
        const bool res = obj->Delete(ctx_, s, strict, ERR);
        SET_TOP(JSVal::Bool(res));
        DISPATCH(DELETE_PROP);
      }

      DEFINE_OPCODE(POP_TOP) {
        POP_UNUSED();
        DISPATCH(POP_TOP);
      }

      DEFINE_OPCODE(POP_TOP_AND_RET) {
        frame->ret_ = POP();
        DISPATCH(POP_TOP_AND_RET);
      }

      DEFINE_OPCODE(POP_N) {
        STACKADJ(static_cast<int>(instr[1].value));
        DISPATCH(POP_N);
      }

      DEFINE_OPCODE(POP_JUMP_IF_FALSE) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        POP_UNUSED();
        if (!x) {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(POP_JUMP_IF_FALSE);
      }

      DEFINE_OPCODE(POP_JUMP_IF_TRUE) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        POP_UNUSED();
        if (x) {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(POP_JUMP_IF_TRUE);
      }

      DEFINE_OPCODE(JUMP_IF_TRUE_OR_POP) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        } else {
          POP_UNUSED();
        }
        DISPATCH(JUMP_IF_TRUE_OR_POP);
      }

      DEFINE_OPCODE(JUMP_IF_FALSE_OR_POP) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        } else {
          POP_UNUSED();
        }
        DISPATCH(JUMP_IF_FALSE_OR_POP);
      }

      DEFINE_OPCODE(JUMP_SUBROUTINE) {
        // calc next address
        PUSH(JSEmpty);
        PUSH(std::distance(frame->data(), instr) +
             OPLength<OP::JUMP_SUBROUTINE>::value);
        PUSH(JSVal::Int32(kJumpFromSubroutine));
        JUMPBY(instr[1].diff);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_RETURN_HOOKED_SUBROUTINE) {
        // calc next address
        PUSH(std::distance(frame->data(), instr) +
             OPLength<OP::JUMP_RETURN_HOOKED_SUBROUTINE>::value);
        PUSH(JSVal::Int32(kJumpFromReturn));
        JUMPBY(instr[1].diff);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_BY) {
        JUMPBY(instr[1].diff);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(ROT_TWO) {
        using std::swap;
        swap(TOP(), SECOND());
        DISPATCH(ROT_TWO);
      }

      DEFINE_OPCODE(ROT_THREE) {
        const JSVal v = TOP();
        TOP() = SECOND();
        SECOND() = THIRD();
        THIRD() = v;
        DISPATCH(ROT_THREE);
      }

      DEFINE_OPCODE(ROT_FOUR) {
        const JSVal v = TOP();
        TOP() = SECOND();
        SECOND() = THIRD();
        THIRD() = FOURTH();
        FOURTH() = v;
        DISPATCH(ROT_FOUR);
      }

      DEFINE_OPCODE(DUP_TOP) {
        const JSVal v = TOP();
        PUSH(v);
        DISPATCH(DUP_TOP);
      }

      DEFINE_OPCODE(DUP_TWO) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        PUSH(w);
        PUSH(v);
        DISPATCH(DUP_TWO);
      }

      DEFINE_OPCODE(PUSH_NULL) {
        PUSH(JSNull);
        DISPATCH(PUSH_NULL);
      }

      DEFINE_OPCODE(PUSH_TRUE) {
        PUSH(JSTrue);
        DISPATCH(PUSH_TRUE);
      }

      DEFINE_OPCODE(PUSH_FALSE) {
        PUSH(JSFalse);
        DISPATCH(PUSH_FALSE);
      }

      DEFINE_OPCODE(PUSH_EMPTY) {
        PUSH(JSEmpty);
        DISPATCH(PUSH_EMPTY);
      }

      DEFINE_OPCODE(PUSH_UNDEFINED) {
        PUSH(JSUndefined);
        DISPATCH(PUSH_UNDEFINED);
      }

      DEFINE_OPCODE(PUSH_THIS) {
        PUSH(frame->GetThis());
        DISPATCH(PUSH_THIS);
      }

      DEFINE_OPCODE(PUSH_UINT32) {
        PUSH(JSVal::UInt32(instr[1].value));
        DISPATCH(PUSH_UINT32);
      }

      DEFINE_OPCODE(PUSH_INT32) {
        PUSH(JSVal::Int32(instr[1].i32));
        DISPATCH(PUSH_INT32);
      }

      DEFINE_OPCODE(UNARY_POSITIVE) {
        const JSVal& v = TOP();
        const double x = v.ToNumber(ctx_, ERR);
        SET_TOP(x);
        DISPATCH(UNARY_POSITIVE);
      }

      DEFINE_OPCODE(UNARY_NEGATIVE) {
        const JSVal& v = TOP();
        const double x = v.ToNumber(ctx_, ERR);
        SET_TOP(-x);
        DISPATCH(UNARY_NEGATIVE);
      }

      DEFINE_OPCODE(UNARY_NOT) {
        const JSVal& v = TOP();
        const bool x = v.ToBoolean(ERR);
        SET_TOP(JSVal::Bool(!x));
        DISPATCH(UNARY_NOT);
      }

      DEFINE_OPCODE(UNARY_BIT_NOT) {
        const JSVal& v = TOP();
        if (v.IsInt32()) {
          SET_TOP(JSVal::Int32(~v.int32()));
        } else {
          const double value = v.ToNumber(ctx_, ERR);
          SET_TOP(JSVal::Int32(~core::DoubleToInt32(value)));
        }
        DISPATCH(UNARY_BIT_NOT);
      }

      DEFINE_OPCODE(TYPEOF) {
        const JSVal& v = TOP();
        const JSVal res = v.TypeOf(ctx_);
        SET_TOP(res);
        DISPATCH(TYPEOF);
      }

      DEFINE_OPCODE(TYPEOF_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        if (JSEnv* current = operation_.GetEnv(frame->lexical_env(), s)) {
          const JSVal expr = current->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr);
          TOP() = expr.TypeOf(ctx_);
        } else {
          // unresolvable reference
          PUSH(ctx_->global_data()->string_undefined());
        }
        DISPATCH(TYPEOF_NAME);
      }

      DEFINE_OPCODE(TYPEOF_HEAP) {
        JSDeclEnv* decl =
            operation_.GetHeapEnv(frame->lexical_env(), instr[3].value);
        assert(decl);
        const JSVal expr = decl->GetByOffset(instr[2].value, strict, ERR);
        PUSH(expr);
        TOP() = expr.TypeOf(ctx_);
        DISPATCH(TYPEOF_HEAP);
      }

      DEFINE_OPCODE(TYPEOF_LOCAL) {
        const JSVal& expr = GETLOCAL(instr[1].value);
        const JSVal res = expr.TypeOf(ctx_);
        PUSH(res);
        DISPATCH(TYPEOF_LOCAL);
      }

      DEFINE_OPCODE(TYPEOF_GLOBAL) {
        const Symbol s = frame->GetName(instr[1].value);
        if (ctx_->global_env()->HasBinding(ctx_, s)) {
          const JSVal expr =
              ctx_->global_env()->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr);
          TOP() = expr.TypeOf(ctx_);
        } else {
          // unresolvable reference
          PUSH(ctx_->global_data()->string_undefined());
        }
        DISPATCH(TYPEOF_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementName<-1, 1>(frame->lexical_env(),
                                            s, strict, ERR);
        PUSH(res);
        DISPATCH(DECREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementName<-1, 0>(frame->lexical_env(),
                                            s, strict, ERR);
        PUSH(res);
        DISPATCH(POSTFIX_DECREMENT_NAME);
      }

      DEFINE_OPCODE(INCREMENT_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementName<1, 1>(frame->lexical_env(),
                                           s, strict, ERR);
        PUSH(res);
        DISPATCH(INCREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementName<1, 0>(frame->lexical_env(),
                                           s, strict, ERR);
        PUSH(res);
        DISPATCH(POSTFIX_INCREMENT_NAME);
      }

      DEFINE_OPCODE(DECREMENT_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementHeap<-1, 1>(
                frame->lexical_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(res);
        DISPATCH(DECREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementHeap<-1, 0>(
                frame->lexical_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(res);
        DISPATCH(POSTFIX_DECREMENT_HEAP);
      }

      DEFINE_OPCODE(INCREMENT_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementHeap<1, 1>(
                frame->lexical_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(res);
        DISPATCH(INCREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.IncrementHeap<1, 0>(
                frame->lexical_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(res);
        DISPATCH(POSTFIX_INCREMENT_HEAP);
      }

      DEFINE_OPCODE(DECREMENT_LOCAL) {
        const JSVal w = GETLOCAL(instr[1].value);
        if (w.IsInt32() && w.int32() > INT32_MIN) {
          const JSVal res = JSVal::Int32(w.int32() - 1);
          SETLOCAL(instr[1].value, res);
          PUSH(res);
        } else {
          const double prev = w.ToNumber(ctx_, ERR);
          const double now = prev - 1;
          SETLOCAL(instr[1].value, now);
          PUSH(prev);
        }
        DISPATCH(DECREMENT_LOCAL);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_LOCAL) {
        const JSVal w = GETLOCAL(instr[1].value);
        if (w.IsInt32() && w.int32() > INT32_MIN) {
          SETLOCAL(instr[1].value, JSVal::Int32(w.int32() - 1));
          PUSH(w);
        } else {
          const double prev = w.ToNumber(ctx_, ERR);
          const double now = prev - 1;
          SETLOCAL(instr[1].value, now);
          PUSH(prev);
        }
        DISPATCH(POSTFIX_DECREMENT_LOCAL);
      }

      DEFINE_OPCODE(INCREMENT_LOCAL) {
        const JSVal w = GETLOCAL(instr[1].value);
        if (w.IsInt32() && w.int32() < INT32_MAX) {
          const JSVal res = JSVal::Int32(w.int32() + 1);
          SETLOCAL(instr[1].value, res);
          PUSH(res);
        } else {
          const double prev = w.ToNumber(ctx_, ERR);
          const double now = prev + 1;
          SETLOCAL(instr[1].value, now);
          PUSH(now);
        }
        DISPATCH(INCREMENT_LOCAL);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_LOCAL) {
        const JSVal w = GETLOCAL(instr[1].value);
        if (w.IsInt32() && w.int32() < INT32_MAX) {
          SETLOCAL(instr[1].value, JSVal::Int32(w.int32() + 1));
          PUSH(w);
        } else {
          const double prev = w.ToNumber(ctx_, ERR);
          const double now = prev + 1;
          SETLOCAL(instr[1].value, now);
          PUSH(prev);
        }
        DISPATCH(POSTFIX_INCREMENT_LOCAL);
      }

      DEFINE_OPCODE(DECREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev - 1;
        PUSH(now);
        DISPATCH(DECREMENT_LOCAL_IMMUTABLE);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        PUSH(prev);
        DISPATCH(POSTFIX_DECREMENT_LOCAL_IMMUTABLE);
      }

      DEFINE_OPCODE(INCREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev + 1;
        PUSH(now);
        DISPATCH(INCREMENT_LOCAL_IMMUTABLE);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        PUSH(prev);
        DISPATCH(POSTFIX_INCREMENT_LOCAL_IMMUTABLE);
      }

      DEFINE_OPCODE(DECREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal val =
            operation_.IncrementGlobal<-1, 1>(global, instr, s, strict, ERR);
        PUSH(val);
        DISPATCH(DECREMENT_GLOBAL);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal val =
            operation_.IncrementGlobal<-1, 0>(global, instr, s, strict, ERR);
        PUSH(val);
        DISPATCH(POSTFIX_DECREMENT_GLOBAL);
      }

      DEFINE_OPCODE(INCREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal val =
            operation_.IncrementGlobal<1, 1>(global, instr, s, strict, ERR);
        PUSH(val);
        DISPATCH(INCREMENT_GLOBAL);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal val =
            operation_.IncrementGlobal<1, 0>(global, instr, s, strict, ERR);
        PUSH(val);
        DISPATCH(POSTFIX_INCREMENT_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT_ELEMENT) {
        const JSVal element = TOP();
        const JSVal base = SECOND();
        SECOND() =
            operation_.IncrementElement<-1, 1>(base, element, strict, ERR);
        POP_UNUSED();
        DISPATCH(DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_ELEMENT) {
        const JSVal element = TOP();
        const JSVal base = SECOND();
        SECOND() =
            operation_.IncrementElement<-1, 0>(base, element, strict, ERR);
        POP_UNUSED();
        DISPATCH(POSTFIX_DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(INCREMENT_ELEMENT) {
        const JSVal element = TOP();
        const JSVal base = SECOND();
        SECOND() =
            operation_.IncrementElement<1, 1>(base, element, strict, ERR);
        POP_UNUSED();
        DISPATCH(INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_ELEMENT) {
        const JSVal element = TOP();
        const JSVal base = SECOND();
        SECOND() =
            operation_.IncrementElement<1, 0>(base, element, strict, ERR);
        POP_UNUSED();
        DISPATCH(POSTFIX_INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(DECREMENT_PROP) {
        const JSVal& base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res = operation_.IncrementProp<-1, 1>(base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH(DECREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_PROP) {
        const JSVal& base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res = operation_.IncrementProp<-1, 0>(base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH(POSTFIX_DECREMENT_PROP);
      }

      DEFINE_OPCODE(INCREMENT_PROP) {
        const JSVal& base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res = operation_.IncrementProp<1, 1>(base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH(INCREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_PROP) {
        const JSVal& base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res = operation_.IncrementProp<1, 0>(base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH(POSTFIX_INCREMENT_PROP);
      }

      DEFINE_OPCODE(DECREMENT_CALL_RESULT)
      DEFINE_OPCODE(POSTFIX_DECREMENT_CALL_RESULT)
      DEFINE_OPCODE(INCREMENT_CALL_RESULT)
      DEFINE_OPCODE(POSTFIX_INCREMENT_CALL_RESULT) {
        TOP().ToNumber(ctx_, ERR);
        e->Report(Error::Reference, "target is not reference");
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(BINARY_ADD) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t sum;
          if (!core::IsAdditionOverflow(lhs.int32(), rhs.int32(), &sum)) {
            SECOND() = JSVal::Int32(sum);
          } else {
            SECOND() =
                JSVal(static_cast<double>(lhs.int32()) +
                      static_cast<double>(rhs.int32()));
          }
        } else {
          SECOND() = operation_.BinaryAdd(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_ADD);
      }

      DEFINE_OPCODE(BINARY_SUBTRACT) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t dif;
          if (!core::IsSubtractOverflow(lhs.int32(), rhs.int32(), &dif)) {
            SECOND() = JSVal::Int32(dif);
          } else {
            SECOND() =
                JSVal(static_cast<double>(lhs.int32()) -
                      static_cast<double>(rhs.int32()));
          }
        } else {
          SECOND() = operation_.BinarySub(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_SUBTRACT);
      }

      DEFINE_OPCODE(BINARY_MULTIPLY) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        // if 1 bit is not found in MSB to MSB - 17 bits,
        // this multiply is safe for overflow
        if (lhs.IsInt32() && rhs.IsInt32() &&
            !((lhs.int32() | rhs.int32()) >> 15)) {
          SECOND() = JSVal::Int32(lhs.int32() * rhs.int32());
        } else {
          SECOND() = operation_.BinaryMultiply(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_MULTIPLY);
      }

      DEFINE_OPCODE(BINARY_DIVIDE) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        SECOND() = operation_.BinaryDivide(lhs, rhs, ERR);
        POP_UNUSED();
        DISPATCH(BINARY_DIVIDE);
      }

      DEFINE_OPCODE(BINARY_MODULO) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        // check rhs is more than 0 (n % 0 == NaN)
        // lhs is >= 0 and rhs is > 0 because example like
        //   -1 % -1
        // should return -0.0, so this value is double
        if (lhs.IsInt32() && rhs.IsInt32() &&
            lhs.int32() >= 0 && rhs.int32() > 0) {
          SECOND() = JSVal::Int32(lhs.int32() % rhs.int32());
        } else {
          SECOND() = operation_.BinaryModulo(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_MODULO);
      }

      DEFINE_OPCODE(BINARY_LSHIFT) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Int32(lhs.int32() << (rhs.int32() & 0x1f));
        } else {
          SECOND() = operation_.BinaryLShift(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_LSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Int32(lhs.int32() >> (rhs.int32() & 0x1f));
        } else {
          SECOND() = operation_.BinaryRShift(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_RSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT_LOGICAL) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        uint32_t left_result;
        if (lhs.GetUInt32(&left_result) && rhs.IsInt32()) {
          SECOND() = JSVal::UInt32(left_result >> (rhs.int32() & 0x1f));
        } else {
          SECOND() = operation_.BinaryRShiftLogical(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_RSHIFT_LOGICAL);
      }

      DEFINE_OPCODE(BINARY_LT) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() < rhs.int32());
        } else {
          SECOND() = operation_.BinaryCompareLT(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_LT);
      }

      DEFINE_OPCODE(BINARY_LTE) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() <= rhs.int32());
        } else {
          SECOND() = operation_.BinaryCompareLTE(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_LTE);
      }

      DEFINE_OPCODE(BINARY_GT) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() > rhs.int32());
        } else {
          SECOND() = operation_.BinaryCompareGT(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_GT);
      }

      DEFINE_OPCODE(BINARY_GTE) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() >= rhs.int32());
        } else {
          SECOND() = operation_.BinaryCompareGTE(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_GTE);
      }

      DEFINE_OPCODE(BINARY_INSTANCEOF) {
        const JSVal w = TOP();
        const JSVal v = SECOND();
        SECOND() = operation_.BinaryInstanceof(v, w, ERR);
        POP_UNUSED();
        DISPATCH(BINARY_INSTANCEOF);
      }

      DEFINE_OPCODE(BINARY_IN) {
        const JSVal w = TOP();
        const JSVal v = SECOND();
        SECOND() = operation_.BinaryIn(v, w, ERR);
        POP_UNUSED();
        DISPATCH(BINARY_IN);
      }

      DEFINE_OPCODE(BINARY_EQ) {
        const JSVal rhs = TOP();
        const JSVal& lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() == rhs.int32());
        } else {
          SECOND() = operation_.BinaryEqual(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_EQ);
      }

      DEFINE_OPCODE(BINARY_STRICT_EQ) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() == rhs.int32());
        } else {
          SECOND() = operation_.BinaryStrictEqual(lhs, rhs);
        }
        POP_UNUSED();
        DISPATCH(BINARY_STRICT_EQ);
      }

      DEFINE_OPCODE(BINARY_NE) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() != rhs.int32());
        } else {
          SECOND() = operation_.BinaryNotEqual(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_NE);
      }

      DEFINE_OPCODE(BINARY_STRICT_NE) {
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Bool(lhs.int32() != rhs.int32());
        } else {
          SECOND() = operation_.BinaryStrictNotEqual(lhs, rhs);
        }
        POP_UNUSED();
        DISPATCH(BINARY_STRICT_NE);
      }

      DEFINE_OPCODE(BINARY_BIT_AND) {  // &
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Int32(lhs.int32() & rhs.int32());
        } else {
          SECOND() = operation_.BinaryBitAnd(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_BIT_AND);
      }

      DEFINE_OPCODE(BINARY_BIT_XOR) {  // ^
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Int32(lhs.int32() ^ rhs.int32());
        } else {
          SECOND() = operation_.BinaryBitXor(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_BIT_XOR);
      }

      DEFINE_OPCODE(BINARY_BIT_OR) {  // |
        const JSVal rhs = TOP();
        const JSVal lhs = SECOND();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SECOND() = JSVal::Int32(lhs.int32() | rhs.int32());
        } else {
          SECOND() = operation_.BinaryBitOr(lhs, rhs, ERR);
        }
        POP_UNUSED();
        DISPATCH(BINARY_BIT_OR);
      }

      DEFINE_OPCODE(RETURN) {
        frame->ret_ = TOP();
        const JSVal ret =
            (frame->constructor_call_ && !frame->ret_.IsObject()) ?
            frame->GetThis() : frame->ret_;
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          return std::make_pair(ret, STATE_RETURN);
        }
        // this code is invoked by JS code
        instr = frame->prev_pc_ + 2;  // EVAL / CALL / CONSTRUCT => 2
        sp = frame->GetPreviousFrameStackTop();
        frame = stack_.Unwind(frame);
        strict = frame->code()->strict();
        PUSH(ret);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(RETURN_SUBROUTINE) {
        const JSVal flag = TOP();
        const JSVal v = SECOND();
        assert(flag.IsInt32());
        const int32_t f = flag.int32();
        if (f == kJumpFromSubroutine) {
          // JUMP_SUBROUTINE
          // return to caller
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          STACKADJ(3);
          DISPATCH_WITH_NO_INCREMENT();
        } else if (f == kJumpFromReturn) {
          // RETURN HOOK
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          STACKADJ(2);
          DISPATCH_WITH_NO_INCREMENT();
        } else {
          // ERROR FINALLY JUMP
          // rethrow error
          assert(f == kJumpFromFinally);
          e->Report(v);
          DISPATCH_ERROR();
        }
      }

      DEFINE_OPCODE(SWITCH_CASE) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        if (JSVal::StrictEqual(v, w)) {
          STACKADJ(2);
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        POP_UNUSED();
        DISPATCH(SWITCH_CASE);
      }

      DEFINE_OPCODE(SWITCH_DEFAULT) {
        POP_UNUSED();
        JUMPBY(instr[1].diff);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(FORIN_SETUP) {
        const JSVal v = TOP();
        if (v.IsNullOrUndefined()) {
          JUMPBY(instr[1].diff);  // skip for-in stmt
          DISPATCH_WITH_NO_INCREMENT();
        }
        JSObject* const obj = v.ToObject(ctx_, ERR);
        SET_TOP(obj);
        NameIterator* it = NameIterator::New(ctx_, obj);
        SET_TOP(JSVal::Cell(it));
        PREDICT(FORIN_SETUP, FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(FORIN_ENUMERATE) {
        NameIterator* it = reinterpret_cast<NameIterator*>(TOP().cell());
        if (it->Has()) {
          const Symbol sym = it->Get();
          it->Next();
          JSString* str = JSString::New(ctx_, sym);
          PUSH(str);
        } else {
          JUMPBY(instr[1].diff);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(THROW) {
        frame->ret_ = POP();
        e->Report(frame->ret_);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(RAISE_IMMUTABLE) {
        const Symbol s = frame->GetName(instr[1].value);
        operation_.RaiseImmutable(s, e);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(DEBUGGER) {
        DISPATCH(DEBUGGER);
      }

      DEFINE_OPCODE(WITH_SETUP) {
        const JSVal val = TOP();
        JSObject* const obj = val.ToObject(ctx_, ERR);
        SET_TOP(obj);
        JSObjectEnv* const with_env =
            JSObjectEnv::New(ctx_, frame->lexical_env(), obj);
        with_env->set_provide_this(true);
        frame->set_lexical_env(with_env);
        frame->dynamic_env_level_ += 1;
        POP_UNUSED();
        DISPATCH(WITH_SETUP);
      }

      DEFINE_OPCODE(POP_ENV) {
        frame->set_lexical_env(frame->lexical_env()->outer());
        frame->dynamic_env_level_ -= 1;
        DISPATCH(POP_ENV);
      }

      DEFINE_OPCODE(TRY_CATCH_SETUP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal error = TOP();
        JSEnv* const catch_env =
            JSStaticEnv::New(ctx_, frame->lexical_env(), s, error);
        frame->set_lexical_env(catch_env);
        frame->dynamic_env_level_ += 1;
        POP_UNUSED();
        DISPATCH(TRY_CATCH_SETUP);
      }

      DEFINE_OPCODE(BUILD_ARRAY) {
        JSArray* ary = JSArray::ReservedNew(ctx_, instr[1].value);
        PUSH(ary);
        DISPATCH(BUILD_ARRAY);
      }

      DEFINE_OPCODE(INIT_VECTOR_ARRAY_ELEMENT) {
        JSArray* ary = static_cast<JSArray*>(SECOND().object());
        ary->SetToVector(instr[1].value, TOP());
        POP_UNUSED();
        DISPATCH(INIT_VECTOR_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(INIT_SPARSE_ARRAY_ELEMENT) {
        JSArray* ary = static_cast<JSArray*>(SECOND().object());
        ary->SetToMap(instr[1].value, TOP());
        POP_UNUSED();
        DISPATCH(INIT_SPARSE_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(BUILD_OBJECT) {
        JSObject* obj = JSObject::New(ctx_, instr[1].map);
        PUSH(obj);
        DISPATCH(BUILD_OBJECT);
      }

      DEFINE_OPCODE(BUILD_FUNCTION) {
        Code* target = frame->code()->codes()[instr[1].value];
        JSFunction* func =
            JSVMFunction::New(ctx_, target, frame->lexical_env());
        PUSH(func);
        DISPATCH(BUILD_FUNCTION);
      }

      DEFINE_OPCODE(BUILD_REGEXP) {
        const JSVal w = TOP();
        JSRegExp* x = JSRegExp::New(ctx_, static_cast<JSRegExp*>(w.object()));
        SET_TOP(x);
        DISPATCH(BUILD_REGEXP);
      }

      DEFINE_OPCODE(STORE_OBJECT_DATA) {
        // opcode | offset | merged
        const JSVal value = TOP();
        const JSVal obj = SECOND();
        assert(obj.object());
        if (instr[2].value) {
          obj.object()->GetSlot(instr[1].value) =
              PropertyDescriptor::Merge(
                  DataDescriptor(value, ATTR::W | ATTR::E | ATTR::C),
              obj.object()->GetSlot(instr[1].value));
        } else {
          obj.object()->GetSlot(instr[1].value) =
              DataDescriptor(value, ATTR::W | ATTR::E | ATTR::C);
        }
        assert(!*e);
        POP_UNUSED();
        DISPATCH(STORE_OBJECT_DATA);
      }

      DEFINE_OPCODE(STORE_OBJECT_GET) {
        // opcode | offset | merged
        const JSVal value = TOP();
        const JSVal obj = SECOND();
        assert(obj.object());
        if (instr[2].value) {
          obj.object()->GetSlot(instr[1].value) =
              PropertyDescriptor::Merge(
                  AccessorDescriptor(value.object(), NULL,
                                     ATTR::E | ATTR::C | ATTR::UNDEF_SETTER),
              obj.object()->GetSlot(instr[1].value));
        } else {
          obj.object()->GetSlot(instr[1].value) =
              AccessorDescriptor(value.object(), NULL,
                                 ATTR::E | ATTR::C | ATTR::UNDEF_SETTER);
        }
        assert(!*e);
        POP_UNUSED();
        DISPATCH(STORE_OBJECT_GET);
      }

      DEFINE_OPCODE(STORE_OBJECT_SET) {
        // opcode | offset | merged
        const JSVal value = TOP();
        const JSVal obj = SECOND();
        assert(obj.object());
        if (instr[2].value) {
          obj.object()->GetSlot(instr[1].value) =
              PropertyDescriptor::Merge(
                  AccessorDescriptor(NULL, value.object(),
                                     ATTR::E | ATTR::C | ATTR::UNDEF_GETTER),
              obj.object()->GetSlot(instr[1].value));
        } else {
          obj.object()->GetSlot(instr[1].value) =
              AccessorDescriptor(NULL, value.object(),
                                 ATTR::E | ATTR::C | ATTR::UNDEF_GETTER);
        }
        assert(!*e);
        POP_UNUSED();
        DISPATCH(STORE_OBJECT_SET);
      }

      DEFINE_OPCODE(CALL) {
        const int argc = instr[1].value;
        const JSVal v = sp[-(argc + 2)];
        if (!v.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func = v.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          Frame* new_frame = stack_.NewCodeFrame(
              ctx_,
              sp,
              static_cast<JSVMFunction*>(func)->code(),
              static_cast<JSVMFunction*>(func)->scope(),
              func,
              instr, argc, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          sp = frame->stacktop();
          strict = frame->code()->strict();
          InitThisBinding(ctx_, frame, ERR);
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        const JSVal x = operation_.Invoke(func, sp, instr[1].value, ERR);
        sp -= (argc + 2);
        PUSH(x);
        DISPATCH(CALL);
      }

      DEFINE_OPCODE(CONSTRUCT) {
        const int argc = static_cast<int>(instr[1].value);
        const JSVal v = sp[-(argc + 2)];
        if (!v.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func = v.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          Code* code = static_cast<JSVMFunction*>(func)->code();
          Frame* new_frame = stack_.NewCodeFrame(
              ctx_,
              sp,
              code,
              static_cast<JSVMFunction*>(func)->scope(),
              func,
              instr, argc, true);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          sp = frame->stacktop();
          strict = code->strict();
          JSObject* const obj = JSObject::New(ctx_, code->ConstructMap(ctx_));
          frame->set_this_binding(obj);
          const JSVal proto = func->Get(ctx_, symbol::prototype(), ERR);
          if (proto.IsObject()) {
            obj->set_prototype(proto.object());
          }
          InitThisBinding(ctx_, frame, ERR);
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        const JSVal x = operation_.Construct(func, sp, instr[1].value, ERR);
        sp -= (argc + 2);
        PUSH(x);
        DISPATCH(CONSTRUCT);
      }

      DEFINE_OPCODE(EVAL) {
        // maybe direct call to eval
        const int argc = instr[1].value;
        const JSVal v = sp[-(argc + 2)];
        if (!v.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func = v.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          Frame* new_frame = stack_.NewCodeFrame(
              ctx_,
              sp,
              static_cast<JSVMFunction*>(func)->code(),
              static_cast<JSVMFunction*>(func)->scope(),
              func,
              instr, argc, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          sp = frame->stacktop();
          strict = frame->code()->strict();
          InitThisBinding(ctx_, frame, ERR);
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        const JSVal x =
            operation_.InvokeMaybeEval(func, sp, instr[1].value, frame, ERR);
        sp -= (argc + 2);
        PUSH(x);
        DISPATCH(EVAL);
      }

      DEFINE_OPCODE(CALL_NAME) {
        const Symbol s = frame->GetName(instr[1].value);
        if (JSEnv* target_env = operation_.GetEnv(frame->lexical_env(), s)) {
          const JSVal res = target_env->GetBindingValue(ctx_, s, false, ERR);
          PUSH(res);
          PUSH(target_env->ImplicitThisValue());
        } else {
          operation_.RaiseReferenceError(s, e);
          DISPATCH_ERROR();
        }
        DISPATCH(CALL_NAME);
      }

      DEFINE_OPCODE(CALL_HEAP) {
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.LoadHeap(frame->lexical_env(), s, strict,
                                instr[2].value, instr[3].value, ERR);
        PUSH(res);
        PUSH(JSUndefined);
        DISPATCH(CALL_HEAP);
      }

      DEFINE_OPCODE(CALL_LOCAL) {
        PUSH(GETLOCAL(instr[1].value));
        PUSH(JSUndefined);
        DISPATCH(CALL_LOCAL);
      }

      DEFINE_OPCODE(CALL_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        if (instr[2].map == global->map()) {
          // map is cached, so use previous index code
          const JSVal val = global->GetBySlotOffset(ctx_, instr[3].value, ERR);
          PUSH(val);
        } else {
          const Symbol s = frame->GetName(instr[1].value);
          Slot slot;
          if (global->GetOwnPropertySlot(ctx_, s, &slot)) {
            // now Own Property Pattern only implemented
            assert(slot.IsCacheable());
            instr[2].map = global->map();
            instr[3].value = slot.offset();
            const JSVal res = slot.Get(ctx_, global, ERR);
            PUSH(res);
          } else {
            const JSVal res =
                operation_.LoadName(ctx_->global_env(), s, strict, ERR);
            PUSH(res);
          }
        }
        PUSH(JSUndefined);
        DISPATCH(CALL_GLOBAL);
      }

      DEFINE_OPCODE(CALL_ELEMENT) {
        const JSVal element = TOP();
        const JSVal base = SECOND();
        SECOND() = operation_.LoadElement(base, element, strict, ERR);
        SET_TOP(base);
        DISPATCH(CALL_ELEMENT);
      }

      DEFINE_OPCODE(CALL_PROP) {
        const JSVal base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res =
            operation_.LoadProp<
              OP::CALL_PROP_OWN,
              OP::CALL_PROP_PROTO,
              OP::CALL_PROP_CHAIN,
              OP::CALL_PROP_GENERIC>(instr, base, s, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        DISPATCH(CALL_PROP);
      }

      DEFINE_OPCODE(CALL_PROP_OWN) {
        // opcode | name | map | offset | nop | nop
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol s = frame->GetName(instr[1].value);
          if (operation_.GetPrimitiveOwnProperty(base, s, &res)) {
            SET_TOP(res);
            DISPATCH(CALL_PROP_OWN);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        if (instr[2].map == obj->map()) {
          // cache hit
          const JSVal res = obj->GetSlot(instr[3].value).Get(ctx_, base, ERR);
          SET_TOP(res);
        } else {
          // uncache
          const Symbol s = frame->GetName(instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::CALL_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        PUSH(base);
        DISPATCH(CALL_PROP_OWN);
      }

      DEFINE_OPCODE(CALL_PROP_PROTO) {
        // opcode | name | map | map | offset | nop
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol s = frame->GetName(instr[1].value);
          if (operation_.GetPrimitiveOwnProperty(base, s, &res)) {
            SET_TOP(res);
            DISPATCH(CALL_PROP_PROTO);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        JSObject* proto = obj->prototype();
        if (instr[2].map == obj->map() &&
            proto && instr[3].map == proto->map()) {
          // cache hit
          const JSVal res = proto->GetSlot(instr[4].value).Get(ctx_, base, ERR);
          SET_TOP(res);
        } else {
          // uncache
          const Symbol s = frame->GetName(instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::CALL_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        PUSH(base);
        DISPATCH(CALL_PROP_PROTO);
      }

      DEFINE_OPCODE(CALL_PROP_CHAIN) {
        // opcode | name | chain | map | offset | nop
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol s = frame->GetName(instr[1].value);
          if (operation_.GetPrimitiveOwnProperty(base, s, &res)) {
            SET_TOP(res);
            DISPATCH(CALL_PROP_CHAIN);
          } else {
            obj = base.GetPrimitiveProto(ctx_);
          }
        } else {
          obj = base.object();
        }
        if (JSObject* cached = instr[2].chain->Validate(obj, instr[3].map)) {
          // cache hit
          const JSVal res =
              cached->GetSlot(instr[4].value).Get(ctx_, base, ERR);
          SET_TOP(res);
        } else {
          // uncache
          const Symbol s = frame->GetName(instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::CALL_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        PUSH(base);
        DISPATCH(CALL_PROP_CHAIN);
      }

      DEFINE_OPCODE(CALL_PROP_GENERIC) {
        const JSVal base = TOP();
        const Symbol s = frame->GetName(instr[1].value);
        const JSVal res = operation_.LoadProp(base, s, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        DISPATCH(CALL_PROP_GENERIC);
      }

      DEFINE_OPCODE(CALL_CALL_RESULT) {
        PUSH(JSUndefined);
        DISPATCH(CALL_CALL_RESULT);
      }

      default: {
        std::printf("%s\n", OP::String(instr->value));
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
        const int handler = std::get<0>(*it);
        const uint16_t begin = std::get<1>(*it);
        const uint16_t end = std::get<2>(*it);
        const uint16_t stack_base_level = std::get<3>(*it);
        const uint16_t env_level = std::get<4>(*it);
        const uint32_t offset = static_cast<uint32_t>(instr - frame->data());
        if (offset < begin) {
          break;  // not found in this exception table
        } else if (offset < end) {
          assert(begin <= offset);
          const JSVal error = JSError::Detail(ctx_, e);
          e->Clear();
          UNWIND_STACK(stack_base_level);
          UNWIND_DYNAMIC_ENV(env_level);
          if (handler == Handler::FINALLY) {
            PUSH(JSEmpty);
          }
          PUSH(error);
          if (handler == Handler::FINALLY) {
            // finally jump if return or error raised
            PUSH(JSVal::Int32(kJumpFromFinally));
          }
          JUMPTO(end);
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
        sp = frame->GetPreviousFrameStackTop();
        frame = stack_.Unwind(frame);
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
#undef PUSH
#undef POP
#undef STACKADJ
#undef UNWIND_STACK
#undef UNWIND_DYNAMIC_ENV
#undef STACK_DEPTH
#undef TOP
#undef SECOND
#undef THIRD
#undef FOURTH
#undef PEEK
#undef SET_TOP
#undef SET_SECOND
#undef SET_THIRD
#undef SET_FOURTH
#undef SET_VALUE
#undef GETLOCAL
#undef SETLOCAL
#undef GETITEM
}  // NOLINT

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_VM_H_
