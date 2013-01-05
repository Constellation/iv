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
#include <iv/lv5/accessor.h>
#include <iv/lv5/railgun/native_iterator.h>
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
#include <iv/lv5/railgun/context_fwd.h>
#include <iv/lv5/radio/radio.h>

namespace iv {
namespace lv5 {
namespace railgun {


Context* VM::ctx() const {
  return static_cast<Context*>(Operation::ctx());
}

// Global
JSVal VM::Run(Code* code, Error* e) {
  return RunEval(code,
                 ctx()->global_env(),
                 ctx()->global_env(),
                 ctx()->global_obj(), e);
}

// Eval
JSVal VM::RunEval(Code* code,
                  JSEnv* variable_env,
                  JSEnv* lexical_env,
                  JSVal this_binding,
                  Error* e) {
  ScopedArguments args(ctx(), 0, IV_LV5_ERROR(e));
  args.set_this_binding(this_binding);
  Frame* frame = stack_.NewEvalFrame(
      ctx(),
      args.ExtractBase(),
      code,
      variable_env,
      lexical_env);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  frame->InitThisBinding(ctx());
  const JSVal res = Execute(frame, e);
#ifdef DEBUG
  if (code->needs_declarative_environment()) {
    assert(frame->lexical_env()->outer() == lexical_env);
  } else {
    assert(frame->lexical_env() == lexical_env);
  }
#endif
  stack_.Unwind(frame);
  return res;
}

JSVal VM::Execute(Arguments* args, JSVMFunction* func, Error* e) {
  Frame* frame = stack_.NewCodeFrame(
      ctx(),
      args->ExtractBase(),
      func->code(),
      func->scope(),
      func,
      NULL,
      args->size() + 1, args->IsConstructorCalled());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  frame->InitThisBinding(ctx());
  const JSVal res = Execute(frame, e);
#ifdef DEBUG
  if (func->code()->needs_declarative_environment()) {
    assert(frame->lexical_env()->outer() == func->scope());
  } else {
    assert(frame->lexical_env() == func->scope());
  }
#endif
  stack_.Unwind(frame);
  return res;
}

JSVal VM::Execute(Frame* start, Error* e) {
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
    return JSEmpty;
#undef V
  }
#endif
  assert(start);
  // current frame values
  Frame* frame = start;
  Instruction* instr = frame->data();
  bool strict = frame->code()->strict();

#define INCREMENT_NEXT(op) (instr += OPLength<OP::op>::value)

#ifdef IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE
// direct threading mode

#ifdef DEBUG
#define DEFINE_OPCODE(op)\
  case OP::op:\
  op:\
    statistics_.Increment(OP::op);
#else
#define DEFINE_OPCODE(op)\
  case OP::op:\
  op:
#endif  // ifdef DEBUG

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

#define PREDICT_WITH_NO_INCREMENT(next) goto next;

#define GO_MAIN_LOOP() DISPATCH_WITH_NO_INCREMENT()

#else
// not direct threading mode
#ifdef DEBUG
#define DEFINE_OPCODE(op)\
  case OP::op:\
    statistics_.Increment(OP::op);
#else
#define DEFINE_OPCODE(op)\
  case OP::op:
#endif  // ifdef DEBUG

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

#define PREDICT_WITH_NO_INCREMENT(next) continue;

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

#define REG(n)\
  (reinterpret_cast<JSVal*>(frame)[FrameConstant<>::kFrameSize + (n)])

// fast opcode macros

#define FAST_PATH_LOAD_CONST()\
  /* opcode | (dst | offset) */\
  REG(instr[1].ssw.i16[0]) = frame->GetConstant(instr[1].ssw.u32);\
  DISPATCH(LOAD_CONST);

#define FAST_PATH_IF_FALSE()\
  if (!REG(instr[1].jump.i16[0]).ToBoolean()) {\
    JUMPBY(instr[1].jump.to);\
    DISPATCH_WITH_NO_INCREMENT();\
  }\
  DISPATCH(IF_FALSE);

#define FAST_PATH_BINARY_LT()\
  const JSVal lhs = REG(instr[1].i16[1]);\
  const JSVal rhs = REG(instr[1].i16[2]);\
  if (lhs.IsInt32() && rhs.IsInt32()) {\
    REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() < rhs.int32());\
  } else {\
    const bool res = BinaryCompareLT(lhs, rhs, ERR);\
    REG(instr[1].i16[0]) = JSVal::Bool(res);\
  }\
  DISPATCH(BINARY_LT);\

#define FAST_PATH_BINARY_ADD()\
  const JSVal lhs = REG(instr[1].i16[1]);\
  const JSVal rhs = REG(instr[1].i16[2]);\
  if (lhs.IsInt32() && rhs.IsInt32()) {\
    int32_t sum;\
    if (!core::math::IsAdditionOverflow(lhs.int32(), rhs.int32(), &sum)) {\
      REG(instr[1].i16[0]) = JSVal::Int32(sum);\
    } else {\
      REG(instr[1].i16[0]) =\
          JSVal(static_cast<double>(lhs.int32()) +\
                static_cast<double>(rhs.int32()));\
    }\
  } else {\
    const JSVal res = BinaryAdd(lhs, rhs, ERR);\
    REG(instr[1].i16[0]) = res;\
  }\
  DISPATCH(BINARY_ADD);

#define FAST_PATH_JUMP_BY()\
  JUMPBY(instr[1].jump.to);\
  DISPATCH_WITH_NO_INCREMENT();

  // main loop
  for (;;) {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
    DISPATCH_WITH_NO_INCREMENT();
#else
#undef DISPATCH_ERROR
#define DISPATCH_ERROR() goto VM_ERROR
 MAIN_LOOP_START:
    // 5 top opcode fast pathes
    // see detail https://github.com/Constellation/iv/issues/63
    if (instr->u32[0] == OP::LOAD_CONST) {
      FAST_PATH_LOAD_CONST();
    } else if (instr->u32[0] == OP::IF_FALSE) {
      FAST_PATH_IF_FALSE();
    } else if (instr->u32[0] == OP::BINARY_LT) {
      FAST_PATH_BINARY_LT();
    } else if (instr->u32[0] == OP::BINARY_ADD) {
      FAST_PATH_BINARY_ADD();
    } else if (instr->u32[0] == OP::JUMP_BY) {
      FAST_PATH_JUMP_BY();
    }
#undef DISPATCH_ERROR
#define DISPATCH_ERROR() break
#endif
    // if ok, use DISPATCH.
    // if error, use DISPATCH_ERROR.
    switch (instr->u32[0]) {
      DEFINE_OPCODE(NOP) {
        // opcode
        DISPATCH(NOP);
      }

      DEFINE_OPCODE(MV) {
        // opcode | (dst | src)
        REG(instr[1].i16[0]) = REG(instr[1].i16[1]);
        DISPATCH(MV);
      }

      DEFINE_OPCODE(RETURN) {
        // opcode | src
        JSVal src = REG(instr[1].i32[0]);
        if (frame->constructor_call_ && !src.IsObject()) {
          src = frame->GetThis();
        }


        // no return at last
        // return undefined
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          return src;
        }
        // this code is invoked by JS code
        // EVAL / CALL / CONSTRUCT

        instr = frame->prev_pc_;

        // because of Frame is code frame,
        // first lexical_env is variable_env.
        // (if Eval / Global, this is not valid)
        assert(frame->lexical_env() == frame->variable_env());

        frame = stack_.Unwind(frame);
        strict = frame->code()->strict();

        ctx()->RAX()  = src;
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(BUILD_ENV) {
        // opcode | (size | mutable_start)
        frame->variable_env_ = frame->lexical_env_ =
            JSDeclEnv::New(ctx(),
                           frame->lexical_env(),
                           instr[1].u32[0],
                           frame->code()->names().begin(),
                           instr[1].u32[1]);
        DISPATCH(BUILD_ENV);
      }

      DEFINE_OPCODE(INSTANTIATE_DECLARATION_BINDING) {
        // opcode | (name | configurable)
        JSEnv* const env = frame->variable_env();
        const Symbol name = frame->GetName(instr[1].u32[0]);
        const bool configurable = instr[1].u32[1];
        if (!env->HasBinding(ctx(), name)) {
          env->CreateMutableBinding(ctx(), name, configurable, ERR);
        } else if (env == ctx()->global_env()) {
          JSObject* const go = ctx()->global_obj();
          const PropertyDescriptor existing_prop = go->GetProperty(ctx(), name);
          if (existing_prop.IsConfigurable()) {
            go->DefineOwnProperty(
                ctx(),
                name,
                DataDescriptor(
                    JSUndefined,
                    ATTR::W | ATTR::E |
                    ((configurable) ? ATTR::C : ATTR::NONE)),
                true, ERR);
          } else {
            if (existing_prop.IsAccessor()) {
              e->Report(Error::Type, "create mutable function binding failed");
              DISPATCH_ERROR();
            }
            const DataDescriptor* const data = existing_prop.AsDataDescriptor();
            if (!data->IsWritable() || !data->IsEnumerable()) {
              e->Report(Error::Type, "create mutable function binding failed");
              DISPATCH_ERROR();
            }
          }
        }
        DISPATCH(INSTANTIATE_DECLARATION_BINDING);
      }

      DEFINE_OPCODE(INSTANTIATE_VARIABLE_BINDING) {
        // opcode | (name | configurable)
        JSEnv* const env = frame->variable_env();
        const Symbol name = frame->GetName(instr[1].u32[0]);
        const bool configurable = instr[1].u32[1];
        if (!env->HasBinding(ctx(), name)) {
          env->CreateMutableBinding(ctx(), name, configurable, ERR);
          env->SetMutableBinding(ctx(), name, JSUndefined, strict, ERR);
        }
        DISPATCH(INSTANTIATE_VARIABLE_BINDING);
      }

      DEFINE_OPCODE(INITIALIZE_HEAP_IMMUTABLE) {
        // opcode | (src | offset)
        JSDeclEnv* const decl = static_cast<JSDeclEnv*>(frame->variable_env());
        const JSVal src = REG(instr[1].ssw.i16[0]);
        decl->InitializeImmutableBinding(instr[1].ssw.u32, src);
        DISPATCH(INITIALIZE_HEAP_IMMUTABLE);
      }

      DEFINE_OPCODE(LOAD_ARGUMENTS) {
        // opcode | dst
        if (!strict) {
          JSObject* obj = JSNormalArguments::New(
              ctx(),
              static_cast<JSFunction*>(frame->callee().object()),
              frame->code()->params(),
              frame->arguments_crbegin(),
              frame->arguments_crend(),
              static_cast<JSDeclEnv*>(frame->variable_env()),
              ERR);
          REG(instr[1].i32[0]) = obj;
        } else {
          JSObject* obj = JSStrictArguments::New(
              ctx(),
              static_cast<JSFunction*>(frame->callee().object()),
              frame->arguments_crbegin(),
              frame->arguments_crend(),
              ERR);
          REG(instr[1].i32[0]) = obj;
        }
        DISPATCH(LOAD_ARGUMENTS);
      }

      DEFINE_OPCODE(LOAD_CONST) {
        // opcode | (dst | offset)
        FAST_PATH_LOAD_CONST();
      }

      DEFINE_OPCODE(LOAD_NAME) {
        // opcode | (dst | index)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res = LoadName(frame->lexical_env(), name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(LOAD_NAME);
      }

      DEFINE_OPCODE(LOAD_HEAP) {
        // opcode | (dst | imm | index) | (offset | nest)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res = LoadHeap(
            frame->lexical_env(),
            name, strict, instr[2].u32[0], instr[2].u32[1], ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(LOAD_HEAP);
      }

      DEFINE_OPCODE(LOAD_GLOBAL) {
        // opcode | (dst | index) | nop | nop
        JSGlobal* global = ctx()->global_obj();
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res = LoadGlobal(global, instr, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(LOAD_GLOBAL);
      }

      DEFINE_OPCODE(LOAD_GLOBAL_DIRECT) {
        // opcode | dst | slot
        StoredSlot* slot = instr[2].slot;
        REG(instr[1].i32[0]) = slot->value();
        DISPATCH(LOAD_GLOBAL_DIRECT);
      }

      DEFINE_OPCODE(LOAD_ELEMENT) {
        // opcode | (dst | base | element)
        const JSVal base = REG(instr[1].i16[1]);
        const JSVal element = REG(instr[1].i16[2]);
        const JSVal res = LoadElement(base, element, strict, ERR);
        REG(instr[1].i16[0]) = res;
        DISPATCH(LOAD_ELEMENT);
      }

      DEFINE_OPCODE(LOAD_PROP) {
        // opcode | (dst | base | name) | nop | nop | nop
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const JSVal res =
            LoadProp<
              OP::LOAD_PROP_OWN,
              OP::LOAD_PROP_PROTO,
              OP::LOAD_PROP_CHAIN,
              OP::LOAD_PROP_GENERIC>(instr, base, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(LOAD_PROP);
      }

      DEFINE_OPCODE(LOAD_PROP_OWN) {
        // opcode | (dst | base | name) | map | offset | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          if (GetPrimitiveOwnProperty(base, name, &res)) {
            REG(instr[1].ssw.i16[0]) = res;
            DISPATCH(LOAD_PROP_OWN);
          } else {
            obj = base.GetPrimitiveProto(ctx());
          }
        } else {
          obj = base.object();
        }
        assert(obj);
        if (instr[2].map == obj->map()) {
          // cache hit
          REG(instr[1].ssw.i16[0]) = obj->Direct(instr[3].u32[0]);
          DISPATCH(LOAD_PROP_OWN);
        } else {
          // not found => uncache
          Slot slot;
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          if (obj->GetPropertySlot(ctx(), name, &slot)) {
            instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
            const JSVal ret = slot.Get(ctx(), obj, ERR);
            REG(instr[1].ssw.i16[0]) = ret;
          } else {
            instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
            REG(instr[1].ssw.i16[0]) = JSUndefined;
          }
        }
        DISPATCH(LOAD_PROP_OWN);
      }

      DEFINE_OPCODE(LOAD_PROP_PROTO) {
        // opcode | (dst | base | name) | map | map | offset
        const JSVal base = REG(instr[1].ssw.i16[1]);
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          if (GetPrimitiveOwnProperty(base, name, &res)) {
            REG(instr[1].ssw.i16[0]) = res;
            DISPATCH(LOAD_PROP_PROTO);
          } else {
            obj = base.GetPrimitiveProto(ctx());
          }
        } else {
          obj = base.object();
        }
        JSObject* proto = obj->prototype();
        if (instr[2].map == obj->map() && proto->map() == instr[3].map) {
          // cache hit
          REG(instr[1].ssw.i16[0]) = proto->Direct(instr[4].u32[0]);
        } else {
          // uncache
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res = LoadProp(base, name, strict, ERR);
          REG(instr[1].ssw.i16[0]) = res;
        }
        DISPATCH(LOAD_PROP_PROTO);
      }

      DEFINE_OPCODE(LOAD_PROP_CHAIN) {
        // opcode | (dst | base | name) | chain | map | offset
        const JSVal base = REG(instr[1].ssw.i16[1]);
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          if (GetPrimitiveOwnProperty(base, name, &res)) {
            REG(instr[1].ssw.i16[0]) = res;
            DISPATCH(LOAD_PROP_CHAIN);
          } else {
            obj = base.GetPrimitiveProto(ctx());
          }
        } else {
          obj = base.object();
        }
        if (JSObject* cached = instr[2].chain->Validate(obj, instr[3].map)) {
          // cache hit
          REG(instr[1].ssw.i16[0]) = cached->Direct(instr[4].u32[0]);
        } else {
          // uncache
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res = LoadProp(base, name, strict, ERR);
          REG(instr[1].ssw.i16[0]) = res;
        }
        DISPATCH(LOAD_PROP_CHAIN);
      }

      DEFINE_OPCODE(LOAD_PROP_GENERIC) {
        // opcode | (dst | base | name) | nop | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res = LoadProp(base, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(LOAD_PROP_GENERIC);
      }

      DEFINE_OPCODE(STORE_NAME) {
        // opcode | (src | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal src = REG(instr[1].ssw.i16[0]);
        StoreName(frame->lexical_env(), name, src, strict, ERR);
        DISPATCH(STORE_NAME);
      }

      DEFINE_OPCODE(STORE_HEAP) {
        // opcode | (src | imm | name) | (offset | nest)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal src = REG(instr[1].ssw.i16[0]);
        StoreHeap(frame->lexical_env(),
                  name, src, strict, instr[2].u32[0], instr[2].u32[1], ERR);
        DISPATCH(STORE_HEAP);
      }

      DEFINE_OPCODE(STORE_GLOBAL) {
        // opcode | (src | name) | nop | nop
        const JSVal src = REG(instr[1].ssw.i16[0]);
        JSGlobal* global = ctx()->global_obj();
        if (instr[2].map == global->map()) {
          // map is cached, so use previous index code
          global->Direct(instr[3].u32[0]) = src;
        } else {
          const Symbol name = frame->GetName(instr[1].ssw.u32);
          assert(!symbol::IsArrayIndexSymbol(name));
          Slot slot;
          if (global->GetOwnNonIndexedPropertySlot(ctx(), name, &slot) && slot.IsStoreCacheable()) {
            instr[2].map = global->map();
            instr[3].u32[0] = slot.offset();
            global->Direct(slot.offset()) = src;
          } else {
            instr[2].map = NULL;
            StoreName(ctx()->global_env(), name, src, strict, ERR);
          }
        }
        DISPATCH(STORE_GLOBAL);
      }

      DEFINE_OPCODE(STORE_GLOBAL_DIRECT) {
        // opcode | src | slot
        const JSVal src = REG(instr[1].i32[0]);
        StoredSlot* slot = instr[2].slot;
        if (slot->attributes().IsWritable()) {
          slot->set_value(src);
        } else {
          if (strict) {
            e->Report(Error::Type, "modifying global variable failed");
            DISPATCH_ERROR();
          }
        }
        DISPATCH(STORE_GLOBAL_DIRECT);
      }

      DEFINE_OPCODE(STORE_ELEMENT) {
        // opcode | (base | element | src)
        const JSVal base = REG(instr[1].i16[0]);
        const JSVal element = REG(instr[1].i16[1]);
        const JSVal src = REG(instr[1].i16[2]);
        StoreElement(base, element, src, strict, ERR);
        DISPATCH(STORE_ELEMENT);
      }

      DEFINE_OPCODE(STORE_PROP) {
        // opcode | (base | src | index) | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[0]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal src = REG(instr[1].ssw.i16[1]);
        StoreProp(base, instr, OP::STORE_PROP_GENERIC, name, src, strict, ERR);
        DISPATCH(STORE_PROP);
      }

      DEFINE_OPCODE(STORE_PROP_GENERIC) {
        // opcode | (base | src | index) | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[0]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal src = REG(instr[1].ssw.i16[1]);
        StoreProp(base, name, src, strict, ERR);
        DISPATCH(STORE_PROP_GENERIC);
      }

      DEFINE_OPCODE(DELETE_NAME) {
        // opcode | (dst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        if (JSEnv* current = GetEnv(frame->lexical_env(), name)) {
          REG(instr[1].ssw.i16[0]) =
              JSVal::Bool(current->DeleteBinding(ctx(), name));
        } else {
          // not found -> unresolvable reference
          REG(instr[1].ssw.i16[0]) = JSTrue;
        }
        DISPATCH(DELETE_NAME);
      }

      DEFINE_OPCODE(DELETE_HEAP) {
        // opcode | (dst | imm | name) | (offset | nest)
        REG(instr[1].ssw.i16[0]) = JSFalse;
        DISPATCH(DELETE_HEAP);
      }

      DEFINE_OPCODE(DELETE_GLOBAL) {
        // opcode | (dst | name) | nop | nop
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        if (ctx()->global_env()->HasBinding(ctx(), name)) {
          const bool res = ctx()->global_env()->DeleteBinding(ctx(), name);
          REG(instr[1].ssw.i16[0]) = JSVal::Bool(res);
        } else {
          // not found -> unresolvable reference
          REG(instr[1].ssw.i16[0]) = JSTrue;
        }
        DISPATCH(DELETE_GLOBAL);
      }

      DEFINE_OPCODE(DELETE_ELEMENT) {
        // opcode | (dst | base | element)
        const JSVal base = REG(instr[1].i16[1]);
        const JSVal element = REG(instr[1].i16[2]);
        base.CheckObjectCoercible(ERR);
        uint32_t index;
        if (element.GetUInt32(&index)) {
          JSObject* const obj = base.ToObject(ctx(), ERR);
          REG(instr[1].i16[0]) = obj;
          const bool result =
              obj->Delete(ctx(),
                          symbol::MakeSymbolFromIndex(index), strict, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(result);
        } else {
          // const radio::Scope scope(ctx());
          const Symbol name = element.ToSymbol(ctx(), ERR);
          JSObject* const obj = base.ToObject(ctx(), ERR);
          REG(instr[1].i16[0]) = obj;
          const bool result = obj->Delete(ctx(), name, strict, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(result);
        }
        DISPATCH(DELETE_ELEMENT);
      }

      DEFINE_OPCODE(DELETE_PROP) {
        // opcode | (dst | base | name) | nop | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        base.CheckObjectCoercible(ERR);
        JSObject* const obj = base.ToObject(ctx(), ERR);
        REG(instr[1].ssw.i16[0]) = obj;
        const bool res = obj->Delete(ctx(), name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = JSVal::Bool(res);
        DISPATCH(DELETE_PROP);
      }

      DEFINE_OPCODE(IF_FALSE) {
        // opcode | jmp | cond
        FAST_PATH_IF_FALSE();
      }

      DEFINE_OPCODE(IF_TRUE) {
        // opcode | (jmp | cond)
        if (REG(instr[1].jump.i16[0]).ToBoolean()) {
          JUMPBY(instr[1].jump.to);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_TRUE);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_LT) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() >= rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareLT(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_LT);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_LT) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() < rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareLT(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_LT);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_LTE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() > rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareLTE(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_LTE);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_LTE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() <= rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareLTE(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_LTE);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_GT) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() <= rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareGT(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_GT);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_GT) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() > rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareGT(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_GT);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_GTE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() < rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareGTE(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_GTE);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_GTE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() >= rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryCompareGTE(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_GTE);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_INSTANCEOF) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        const bool res = BinaryInstanceof(lhs, rhs, ERR);
        if (!res) {
          JUMPBY(instr[1].jump.to);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_FALSE_BINARY_INSTANCEOF);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_INSTANCEOF) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        const bool res = BinaryInstanceof(lhs, rhs, ERR);
        if (res) {
          JUMPBY(instr[1].jump.to);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_TRUE_BINARY_INSTANCEOF);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_IN) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        const bool res = BinaryIn(lhs, rhs, ERR);
        if (!res) {
          JUMPBY(instr[1].jump.to);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_FALSE_BINARY_IN);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_IN) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        const bool res = BinaryIn(lhs, rhs, ERR);
        if (res) {
          JUMPBY(instr[1].jump.to);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(IF_TRUE_BINARY_IN);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_EQ) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() != rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryEqual(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_EQ);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_EQ) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() == rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryEqual(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_EQ);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_NE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() == rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryNotEqual(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_NE);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_NE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() != rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryNotEqual(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_NE);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_STRICT_EQ) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() != rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryStrictEqual(lhs, rhs);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_STRICT_EQ);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_STRICT_EQ) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() == rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryStrictEqual(lhs, rhs);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_STRICT_EQ);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_STRICT_NE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() == rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryStrictNotEqual(lhs, rhs);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_STRICT_NE);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_STRICT_NE) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() != rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const bool res = BinaryStrictNotEqual(lhs, rhs);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_STRICT_NE);
      }

      DEFINE_OPCODE(IF_FALSE_BINARY_BIT_AND) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (!(lhs.int32() & rhs.int32())) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const int32_t res = BinaryBitAnd(lhs, rhs, ERR);
          if (!res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_FALSE_BINARY_BIT_AND);
      }

      DEFINE_OPCODE(IF_TRUE_BINARY_BIT_AND) {
        // opcode | (jmp | lhs | rhs)
        const JSVal lhs = REG(instr[1].jump.i16[0]);
        const JSVal rhs = REG(instr[1].jump.i16[1]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          if (lhs.int32() & rhs.int32()) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        } else {
          const int32_t res = BinaryBitAnd(lhs, rhs, ERR);
          if (res) {
            JUMPBY(instr[1].jump.to);
            DISPATCH_WITH_NO_INCREMENT();
          }
        }
        DISPATCH(IF_TRUE_BINARY_BIT_AND);
      }

      DEFINE_OPCODE(JUMP_SUBROUTINE) {
        // opcode | (jmp : addr | flag)
        REG(instr[1].jump.i16[0]) =
            ((std::distance(frame->data(), instr) +
              OPLength<OP::JUMP_SUBROUTINE>::value));
        REG(instr[1].jump.i16[1]) = JSVal::Int32(kJumpFromSubroutine);
        JUMPBY(instr[1].jump.to);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_BY) {
        // opcode | jmp
        FAST_PATH_JUMP_BY();
      }

      DEFINE_OPCODE(UNARY_POSITIVE) {
        // opcode | (dst | src)
        const JSVal src = REG(instr[1].i16[1]);
        if (src.IsNumber()) {
          REG(instr[1].i16[0]) = src;
        } else {
          const double x = src.ToNumber(ctx(), ERR);
          REG(instr[1].i16[0]) = x;
        }
        DISPATCH(UNARY_POSITIVE);
      }

      DEFINE_OPCODE(UNARY_NEGATIVE) {
        // opcode | (dst | src)
        const JSVal src = REG(instr[1].i16[1]);
        const double x = src.ToNumber(ctx(), ERR);
        REG(instr[1].i16[0]) = (-x);
        DISPATCH(UNARY_NEGATIVE);
      }

      DEFINE_OPCODE(UNARY_NOT) {
        // opcode | (dst | src)
        const JSVal src = REG(instr[1].i16[1]);
        REG(instr[1].i16[0]) = JSVal::Bool(!src.ToBoolean());
        DISPATCH(UNARY_NOT);
      }

      DEFINE_OPCODE(UNARY_BIT_NOT) {
        // opcode | (dst | src)
        const JSVal src = REG(instr[1].i16[1]);
        if (src.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Int32(~src.int32());
        } else {
          const double value = src.ToNumber(ctx(), ERR);
          REG(instr[1].i16[0]) = JSVal::Int32(~core::DoubleToInt32(value));
        }
        DISPATCH(UNARY_BIT_NOT);
      }

      DEFINE_OPCODE(TYPEOF) {
        // opcode | (dst | src)
        REG(instr[1].i16[0]) = REG(instr[1].i16[1]).TypeOf(ctx());
        DISPATCH(TYPEOF);
      }

      DEFINE_OPCODE(TYPEOF_NAME) {
        // opcode | (dst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        if (JSEnv* current = GetEnv(frame->lexical_env(), name)) {
          const JSVal res = current->GetBindingValue(ctx(), name, strict, ERR);
          REG(instr[1].ssw.i16[0]) = res.TypeOf(ctx());
        } else {
          // unresolvable reference
          REG(instr[1].ssw.i16[0]) = ctx()->global_data()->string_undefined();
        }
        DISPATCH(TYPEOF_NAME);
      }

      DEFINE_OPCODE(TYPEOF_HEAP) {
        // opcode | (dst | imm | name) | (offset | nest)
        JSDeclEnv* decl = GetHeapEnv(frame->lexical_env(), instr[2].u32[1]);
        assert(decl);
        const JSVal res = decl->GetByOffset(instr[2].u32[0], strict, ERR);
        REG(instr[1].ssw.i16[0]) = res.TypeOf(ctx());
        DISPATCH(TYPEOF_HEAP);
      }

      DEFINE_OPCODE(TYPEOF_GLOBAL) {
        // opcode | (dst | name) | nop | nop
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        if (ctx()->global_env()->HasBinding(ctx(), name)) {
          const JSVal res =
              ctx()->global_env()->GetBindingValue(ctx(), name, strict, ERR);
          REG(instr[1].ssw.i16[0]) = res.TypeOf(ctx());
        } else {
          // unresolvable reference
          REG(instr[1].ssw.i16[0]) = ctx()->global_data()->string_undefined();
        }
        DISPATCH(TYPEOF_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT) {
        // opcode | src
        const JSVal src = REG(instr[1].i32[0]);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<-1>(src.int32())) {
          REG(instr[1].i32[0]) = src.int32() - 1;
        } else {
          const double res = src.ToNumber(ctx(), ERR);
          REG(instr[1].i32[0]) = res - 1;
        }
        DISPATCH(DECREMENT);
      }

      DEFINE_OPCODE(INCREMENT) {
        // opcode | src
        const JSVal src = REG(instr[1].i32[0]);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<+1>(src.int32())) {
          REG(instr[1].i32[0]) = src.int32() + 1;
        } else {
          const double res = src.ToNumber(ctx(), ERR);
          REG(instr[1].i32[0]) = res + 1;
        }
        DISPATCH(INCREMENT);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT) {
        // opcode | (dst | src)
        const JSVal src = REG(instr[1].i16[1]);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<-1>(src.int32())) {
          REG(instr[1].i16[0]) = src;
          REG(instr[1].i16[1]) = src.int32() - 1;
        } else {
          const double res = src.ToNumber(ctx(), ERR);
          REG(instr[1].i16[0]) = res;
          REG(instr[1].i16[1]) = res - 1;
        }
        DISPATCH(POSTFIX_DECREMENT);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT) {
        // opcode | (dst | src)
        const JSVal src = REG(instr[1].i16[1]);
        if (src.IsInt32() && detail::IsIncrementOverflowSafe<+1>(src.int32())) {
          REG(instr[1].i16[0]) = src;
          REG(instr[1].i16[1]) = src.int32() + 1;
        } else {
          const double res = src.ToNumber(ctx(), ERR);
          REG(instr[1].i16[0]) = res;
          REG(instr[1].i16[1]) = res + 1;
        }
        DISPATCH(POSTFIX_INCREMENT);
      }

      DEFINE_OPCODE(DECREMENT_NAME) {
        // opcode | (dst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementName<-1, 1>(frame->lexical_env(), name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(DECREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_NAME) {
        // opcode | (dst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementName<-1, 0>(frame->lexical_env(), name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(POSTFIX_DECREMENT_NAME);
      }

      DEFINE_OPCODE(INCREMENT_NAME) {
        // opcode | (dst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementName<1, 1>(frame->lexical_env(), name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(INCREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_NAME) {
        // opcode | (dst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementName<1, 0>(frame->lexical_env(), name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(POSTFIX_INCREMENT_NAME);
      }

      DEFINE_OPCODE(DECREMENT_HEAP) {
        // opcode | (dst | imm | name) | (offset | nest)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementHeap<-1, 1>(
                frame->lexical_env(), name, strict,
                instr[2].u32[0], instr[2].u32[1], ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(DECREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_HEAP) {
        // opcode | (dst | imm | name) | (offset | nest)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementHeap<-1, 0>(
                frame->lexical_env(), name, strict,
                instr[2].u32[0], instr[2].u32[1], ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(POSTFIX_DECREMENT_HEAP);
      }

      DEFINE_OPCODE(INCREMENT_HEAP) {
        // opcode | (dst | imm | name) | (offset | nest)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementHeap<1, 1>(
                frame->lexical_env(), name, strict,
                instr[2].u32[0], instr[2].u32[1], ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(INCREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_HEAP) {
        // opcode | (dst | imm | name) | (offset | nest)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementHeap<1, 0>(
                frame->lexical_env(), name, strict,
                instr[2].u32[0], instr[2].u32[1], ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(POSTFIX_INCREMENT_HEAP);
      }

      DEFINE_OPCODE(DECREMENT_ELEMENT) {
        // opcode | (dst | base | element)
        const JSVal base = REG(instr[1].i16[1]);
        const JSVal element = REG(instr[1].i16[2]);
        const JSVal res =
            IncrementElement<-1, 1>(base, element, strict, ERR);
        REG(instr[1].i16[0]) = res;
        DISPATCH(DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_ELEMENT) {
        // opcode | (dst | base | element)
        const JSVal base = REG(instr[1].i16[1]);
        const JSVal element = REG(instr[1].i16[2]);
        const JSVal res =
            IncrementElement<-1, 0>(base, element, strict, ERR);
        REG(instr[1].i16[0]) = res;
        DISPATCH(POSTFIX_DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(INCREMENT_ELEMENT) {
        // opcode | (dst | base | element)
        const JSVal base = REG(instr[1].i16[1]);
        const JSVal element = REG(instr[1].i16[2]);
        const JSVal res =
            IncrementElement<1, 1>(base, element, strict, ERR);
        REG(instr[1].i16[0]) = res;
        DISPATCH(INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_ELEMENT) {
        // opcode | (dst | base | element)
        const JSVal base = REG(instr[1].i16[1]);
        const JSVal element = REG(instr[1].i16[2]);
        const JSVal res =
            IncrementElement<1, 0>(base, element, strict, ERR);
        REG(instr[1].i16[0]) = res;
        DISPATCH(POSTFIX_INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(DECREMENT_PROP) {
        // opcode | (dst | base | name) | nop | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementProp<-1, 1>(base, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(DECREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_PROP) {
        // opcode | (dst | base | name) | nop | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementProp<-1, 0>(base, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(POSTFIX_DECREMENT_PROP);
      }

      DEFINE_OPCODE(INCREMENT_PROP) {
        // opcode | (dst | base | name) | nop | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementProp<1, 1>(base, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(INCREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_PROP) {
        // opcode | (dst | base | name) | nop | nop | nop
        const JSVal base = REG(instr[1].ssw.i16[1]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        const JSVal res =
            IncrementProp<1, 0>(base, name, strict, ERR);
        REG(instr[1].ssw.i16[0]) = res;
        DISPATCH(POSTFIX_INCREMENT_PROP);
      }

      DEFINE_OPCODE(BINARY_ADD) {
        // opcode | (dst | lhs | rhs)
        FAST_PATH_BINARY_ADD();
      }

      DEFINE_OPCODE(BINARY_SUBTRACT) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t dif;
          if (!core::math::IsSubtractOverflow(lhs.int32(), rhs.int32(), &dif)) {
            REG(instr[1].i16[0]) = JSVal::Int32(dif);
          } else {
            REG(instr[1].i16[0]) =
                JSVal(static_cast<double>(lhs.int32()) -
                      static_cast<double>(rhs.int32()));
          }
        } else {
          const JSVal res = BinarySub(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = res;
        }
        DISPATCH(BINARY_SUBTRACT);
      }

      DEFINE_OPCODE(BINARY_MULTIPLY) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        // if 1 bit is not found in MSB to MSB - 17 bits,
        // this multiply is safe for overflow
        if (lhs.IsInt32() && rhs.IsInt32() &&
            !((lhs.int32() | rhs.int32()) >> 15)) {
          REG(instr[1].i16[0]) = JSVal::Int32(lhs.int32() * rhs.int32());
        } else {
          const JSVal res = BinaryMultiply(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = res;
        }
        DISPATCH(BINARY_MULTIPLY);
      }

      DEFINE_OPCODE(BINARY_DIVIDE) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        const JSVal res = BinaryDivide(lhs, rhs, ERR);
        REG(instr[1].i16[0]) = res;
        DISPATCH(BINARY_DIVIDE);
      }

      DEFINE_OPCODE(BINARY_MODULO) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        // check rhs is more than 0 (n % 0 == NaN)
        // lhs is >= 0 and rhs is > 0 because example like
        //   -1 % -1
        // should return -0.0, so this value is double
        if (lhs.IsInt32() && rhs.IsInt32() &&
            lhs.int32() >= 0 && rhs.int32() > 0) {
          REG(instr[1].i16[0]) = JSVal::Int32(lhs.int32() % rhs.int32());
        } else {
          const JSVal res = BinaryModulo(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = res;
        }
        DISPATCH(BINARY_MODULO);
      }

      DEFINE_OPCODE(BINARY_LSHIFT) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) =
              JSVal::Int32(lhs.int32() << (rhs.int32() & 0x1f));
        } else {
          const JSVal res = BinaryLShift(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = res;
        }
        DISPATCH(BINARY_LSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) =
              JSVal::Int32(lhs.int32() >> (rhs.int32() & 0x1f));
        } else {
          const JSVal res = BinaryRShift(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = res;
        }
        DISPATCH(BINARY_RSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT_LOGICAL) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        uint32_t left_result;
        if (lhs.GetUInt32(&left_result) && rhs.IsInt32()) {
          REG(instr[1].i16[0]) =
              JSVal::UInt32(left_result >> (rhs.int32() & 0x1f));
        } else {
          const JSVal res = BinaryRShiftLogical(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = res;
        }
        DISPATCH(BINARY_RSHIFT_LOGICAL);
      }

      DEFINE_OPCODE(BINARY_LT) {
        // opcode | (dst | lhs | rhs)
        FAST_PATH_BINARY_LT();
      }

      DEFINE_OPCODE(BINARY_LTE) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() <= rhs.int32());
        } else {
          const bool res = BinaryCompareLTE(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_LTE);
      }

      DEFINE_OPCODE(BINARY_GT) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() > rhs.int32());
        } else {
          const bool res = BinaryCompareGT(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_GT);
      }

      DEFINE_OPCODE(BINARY_GTE) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() >= rhs.int32());
        } else {
          const bool res = BinaryCompareGTE(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_GTE);
      }

      DEFINE_OPCODE(BINARY_INSTANCEOF) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        const bool res = BinaryInstanceof(lhs, rhs, ERR);
        REG(instr[1].i16[0]) = JSVal::Bool(res);
        DISPATCH(BINARY_INSTANCEOF);
      }

      DEFINE_OPCODE(BINARY_IN) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        const bool res = BinaryIn(lhs, rhs, ERR);
        REG(instr[1].i16[0]) = JSVal::Bool(res);
        DISPATCH(BINARY_IN);
      }

      DEFINE_OPCODE(BINARY_EQ) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() == rhs.int32());
        } else {
          const bool res = BinaryEqual(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_EQ);
      }

      DEFINE_OPCODE(BINARY_STRICT_EQ) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() == rhs.int32());
        } else {
          const bool res = BinaryStrictEqual(lhs, rhs);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_STRICT_EQ);
      }

      DEFINE_OPCODE(BINARY_NE) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() != rhs.int32());
        } else {
          const bool res = BinaryNotEqual(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_NE);
      }

      DEFINE_OPCODE(BINARY_STRICT_NE) {
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Bool(lhs.int32() != rhs.int32());
        } else {
          const bool res = BinaryStrictNotEqual(lhs, rhs);
          REG(instr[1].i16[0]) = JSVal::Bool(res);
        }
        DISPATCH(BINARY_STRICT_NE);
      }

      DEFINE_OPCODE(BINARY_BIT_AND) {  // &
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Int32(lhs.int32() & rhs.int32());
        } else {
          const int32_t res = BinaryBitAnd(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Int32(res);
        }
        DISPATCH(BINARY_BIT_AND);
      }

      DEFINE_OPCODE(BINARY_BIT_XOR) {  // ^
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Int32(lhs.int32() ^ rhs.int32());
        } else {
          const int32_t res = BinaryBitXor(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Int32(res);
        }
        DISPATCH(BINARY_BIT_XOR);
      }

      DEFINE_OPCODE(BINARY_BIT_OR) {  // |
        // opcode | (dst | lhs | rhs)
        const JSVal lhs = REG(instr[1].i16[1]);
        const JSVal rhs = REG(instr[1].i16[2]);
        if (lhs.IsInt32() && rhs.IsInt32()) {
          REG(instr[1].i16[0]) = JSVal::Int32(lhs.int32() | rhs.int32());
        } else {
          const int32_t res = BinaryBitOr(lhs, rhs, ERR);
          REG(instr[1].i16[0]) = JSVal::Int32(res);
        }
        DISPATCH(BINARY_BIT_OR);
      }

      DEFINE_OPCODE(RETURN_SUBROUTINE) {
        // opcode | (jmp | flag)
        const JSVal v = REG(instr[1].i16[0]);
        const JSVal flag = REG(instr[1].i16[1]);
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
        // opcode | (jmp : iterator | enumerable)
        // TODO(Constellation): fix this for Exact GC
        const JSVal enumerable = REG(instr[1].jump.i16[1]);
        if (enumerable.IsNullOrUndefined()) {
          JUMPBY(instr[1].jump.to);  // skip for-in stmt
          DISPATCH_WITH_NO_INCREMENT();
        }
        NativeIterator* it;
        if (enumerable.IsString()) {
          it = ctx()->GainNativeIterator(enumerable.string());
        } else {
          JSObject* const obj = enumerable.ToObject(ctx(), ERR);
          it = ctx()->GainNativeIterator(obj);
        }
        REG(instr[1].jump.i16[0]) = JSVal::Cell(it);
        PREDICT(FORIN_SETUP, FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(FORIN_ENUMERATE) {
        // opcode | (jmp : dst | iterator)
        NativeIterator* it =
            static_cast<NativeIterator*>(REG(instr[1].jump.i16[1]).cell());
        const Symbol sym = it->Next();
        if (sym != symbol::kDummySymbol) {
          REG(instr[1].jump.i16[0]) = JSString::New(ctx(), sym);
        } else {
          JUMPBY(instr[1].jump.to);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(FORIN_LEAVE) {
        // opcode | iterator
        NativeIterator* it =
            static_cast<NativeIterator*>(REG(instr[1].i32[0]).cell());
        ctx()->ReleaseNativeIterator(it);
        DISPATCH(FORIN_LEAVE);
      }

      DEFINE_OPCODE(THROW) {
        // opcode | src
        e->Report(REG(instr[1].i32[0]));
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(RAISE) {
        // opcode | (code | message)
        const Error::Code code = static_cast<Error::Code>(instr[1].u32[0]);
        const uint32_t constant = instr[1].u32[1];
        JSString* str = frame->GetConstant(constant).string();
        e->Report(code, str->GetUString());
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(TO_NUMBER) {
        // opcode | src
        const JSVal src = REG(instr[1].i32[0]);
        src.ToNumber(ctx(), ERR);
        DISPATCH(TO_NUMBER);
      }

      DEFINE_OPCODE(TO_PRIMITIVE_AND_TO_STRING) {
        // opcode | src
        const JSVal src = REG(instr[1].i32[0]);
        const JSVal primitive = src.ToPrimitive(ctx(), Hint::NONE, ERR);
        JSString* str = primitive.ToString(ctx(), ERR);
        REG(instr[1].i32[0]) = str;
        DISPATCH(TO_PRIMITIVE_AND_TO_STRING);
      }

      DEFINE_OPCODE(CONCAT) {
        // opcode | (dst | start | count)
        JSVal* src = &REG(instr[1].ssw.i16[1]);
        JSString* str = JSString::New(ctx(), src, instr[1].ssw.u32, ERR);
        REG(instr[1].ssw.i16[0]) = str;
        DISPATCH(CONCAT);
      }

      DEFINE_OPCODE(DEBUGGER) {
        // opcode
        DISPATCH(DEBUGGER);
      }

      DEFINE_OPCODE(WITH_SETUP) {
        // opcode | src
        // TODO(Constellation): fix this for Exact GC
        const JSVal src = REG(instr[1].i32[0]);
        JSObject* const obj = src.ToObject(ctx(), ERR);
        JSObjectEnv* const with_env =
            JSObjectEnv::New(ctx(), frame->lexical_env(), obj);
        with_env->set_provide_this(true);
        frame->set_lexical_env(with_env);
        DISPATCH(WITH_SETUP);
      }

      DEFINE_OPCODE(POP_ENV) {
        // opcode
#ifdef DEBUG
        VerifyDynamicEnvironment(frame);
#endif
        frame->set_lexical_env(frame->lexical_env()->outer());
        DISPATCH(POP_ENV);
      }

      DEFINE_OPCODE(TRY_CATCH_SETUP) {
        // opcode | (error | name)
        const JSVal error = REG(instr[1].ssw.i16[0]);
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        JSEnv* const catch_env =
            JSStaticEnv::New(ctx(), frame->lexical_env(), name, error);
        frame->set_lexical_env(catch_env);
        DISPATCH(TRY_CATCH_SETUP);
      }

      DEFINE_OPCODE(LOAD_ARRAY) {
        // opcode | (dst | size)
        REG(instr[1].ssw.i16[0]) =
            JSArray::ReservedNew(ctx(), instr[1].ssw.u32);
        DISPATCH(LOAD_ARRAY);
      }

      DEFINE_OPCODE(DUP_ARRAY) {
        // opcode | (dst | offset)
        REG(instr[1].ssw.i16[0]) =
            JSArray::New(
                ctx(),
                static_cast<JSArray*>(frame->GetConstant(instr[1].ssw.u32).object()));
        DISPATCH(DUP_ARRAY);
      }

      DEFINE_OPCODE(INIT_VECTOR_ARRAY_ELEMENT) {
        // opcode | (ary | reg) | (index | size)
        JSArray* ary = static_cast<JSArray*>(REG(instr[1].i16[0]).object());
        const JSVal* reg = &REG(instr[1].i16[1]);
        ary->SetToVector(instr[2].u32[0], reg, reg + instr[2].u32[1]);
        DISPATCH(INIT_VECTOR_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(INIT_SPARSE_ARRAY_ELEMENT) {
        // opcode | (ary | reg) | (index | size)
        JSArray* ary = static_cast<JSArray*>(REG(instr[1].i16[0]).object());
        const JSVal* reg = &REG(instr[1].i16[1]);
        ary->SetToMap(instr[2].u32[0], reg, reg + instr[2].u32[1]);
        DISPATCH(INIT_SPARSE_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(LOAD_OBJECT) {
        // opcode | dst | map
        REG(instr[1].i32[0]) = JSObject::New(ctx(), instr[2].map);
        DISPATCH(LOAD_OBJECT);
      }

      DEFINE_OPCODE(LOAD_FUNCTION) {
        // opcode | (dst | code)
        Code* target = frame->code()->codes()[instr[1].ssw.u32];
        REG(instr[1].ssw.i16[0]) =
            JSVMFunction::New(ctx(), target, frame->lexical_env());
        DISPATCH(LOAD_FUNCTION);
      }

      DEFINE_OPCODE(LOAD_REGEXP) {
        // opcode | (dst | offset)
        REG(instr[1].ssw.i16[0]) = JSRegExp::New(
            ctx(),
            static_cast<JSRegExp*>(
                frame->GetConstant(instr[1].ssw.u32).object()));
        DISPATCH(LOAD_REGEXP);
      }

      DEFINE_OPCODE(STORE_OBJECT_INDEXED) {
        // opcode | (obj | item) | (index | type)
        assert(REG(instr[1].i16[0]).IsObject());
        JSObject* const obj = REG(instr[1].i16[0]).object();
        const JSVal item = REG(instr[1].i16[0]);
        const uint32_t index = instr[2].u32[0];
        const uint32_t type = instr[2].u32[1];
        Slot slot;
        switch (type) {
          case ObjectLiteral::DATA:
            obj->DefineOwnIndexedPropertySlot(
                ctx(), index,
                DataDescriptor(
                    item,
                    ATTR::W | ATTR::E| ATTR::C),
                &slot, false, e);
            break;
          case ObjectLiteral::GET:
            obj->DefineOwnIndexedPropertySlot(
                ctx(), index,
                AccessorDescriptor(
                    item.object(),
                    NULL,
                    ATTR::E| ATTR::C|
                    ATTR::UNDEF_SETTER),
                &slot, false, e);
            break;
          case ObjectLiteral::SET:
            obj->DefineOwnIndexedPropertySlot(
                ctx(), index,
                AccessorDescriptor(
                    NULL,
                    item.object(),
                    ATTR::E| ATTR::C|
                    ATTR::UNDEF_GETTER),
                &slot, false, e);
            break;
        }
        DISPATCH(STORE_OBJECT_INDEXED);
      }

      DEFINE_OPCODE(STORE_OBJECT_DATA) {
        // opcode | (obj | item) | (offset | merged)
        assert(REG(instr[1].i16[0]).IsObject());
        JSObject* const obj = REG(instr[1].i16[0]).object();
        obj->Direct(instr[2].u32[0]) = REG(instr[1].i16[1]);
        DISPATCH(STORE_OBJECT_DATA);
      }

      DEFINE_OPCODE(STORE_OBJECT_GET) {
        // opcode | (obj | item) | (offset | merged)
        JSObject* const obj = REG(instr[1].i16[0]).object();
        const JSVal value = REG(instr[1].i16[1]);
        if (instr[2].u32[1]) {
          Accessor* ac = static_cast<Accessor*>(obj->Direct(instr[2].u32[0]).cell());
          ac->set_getter(value.object());
        } else {
          obj->Direct(instr[2].u32[0]) = JSVal::Cell(Accessor::New(ctx(), value.object(), NULL));
        }
        DISPATCH(STORE_OBJECT_GET);
      }

      DEFINE_OPCODE(STORE_OBJECT_SET) {
        // opcode | (obj | item) | (offset | merged)
        JSObject* const obj = REG(instr[1].i16[0]).object();
        const JSVal value = REG(instr[1].i16[1]);
        if (instr[2].u32[1]) {
          Accessor* ac = static_cast<Accessor*>(obj->Direct(instr[2].u32[0]).cell());
          ac->set_setter(value.object());
        } else {
          obj->Direct(instr[2].u32[0]) = JSVal::Cell(Accessor::New(ctx(), NULL, value.object()));
        }
        DISPATCH(STORE_OBJECT_SET);
      }

      DEFINE_OPCODE(CALL) {
        // opcode | (callee | offset | argc_with_this)
        const JSVal callee = REG(instr[1].ssw.i16[0]);
        JSVal* offset = &REG(instr[1].ssw.i16[1]);
        const uint32_t argc_with_this = instr[1].ssw.u32;
        if (!callee.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func =
            static_cast<JSFunction*>(callee.object());
        if (func->function_type() == JSFunction::FUNCTION_USER) {
          // inline call
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          Code* code = vm_func->code();
          if (code->empty()) {
            ctx()->RAX() = JSUndefined;
            DISPATCH(CALL);
          }
          Frame* new_frame = stack_.NewCodeFrame(
              ctx(),
              offset,
              code,
              vm_func->scope(),
              func,
              instr + OPLength<OP::CALL>::value,
              argc_with_this, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          strict = frame->code()->strict();
          frame->InitThisBinding(ctx());
          DISPATCH_WITH_NO_INCREMENT();
        }
        // Native Function, so use Invoke
        ctx()->RAX() = Invoke(func, offset, argc_with_this, ERR);
        DISPATCH(CALL);
      }

      DEFINE_OPCODE(CONSTRUCT) {
        // opcode | (callee | offset | argc_with_this)
        const JSVal callee = REG(instr[1].ssw.i16[0]);
        JSVal* offset = &REG(instr[1].ssw.i16[1]);
        const uint32_t argc_with_this = instr[1].ssw.u32;
        if (!callee.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func =
            static_cast<JSFunction*>(callee.object());
        if (func->function_type() == JSFunction::FUNCTION_USER) {
          // inline call
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          Code* code = vm_func->code();
          Frame* new_frame = stack_.NewCodeFrame(
              ctx(),
              offset,
              code,
              vm_func->scope(),
              func,
              instr + OPLength<OP::CONSTRUCT>::value,
              argc_with_this, true);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          strict = code->strict();
          Map* map = func->construct_map(ctx(), ERR);
          JSObject* const obj = JSObject::New(ctx(), map);
          frame->set_this_binding(obj);
          frame->InitThisBinding(ctx());
          DISPATCH_WITH_NO_INCREMENT();
        }

        // Native Function, so use Invoke
        ctx()->RAX() = Construct(func, offset, argc_with_this, ERR);
        DISPATCH(CONSTRUCT);
      }

      DEFINE_OPCODE(EVAL) {
        // opcode | (callee | offset | argc_with_this)
        // maybe direct call to eval
        const JSVal callee = REG(instr[1].ssw.i16[0]);
        JSVal* offset = &REG(instr[1].ssw.i16[1]);
        const uint32_t argc_with_this = instr[1].ssw.u32;
        if (!callee.IsCallable()) {
          e->Report(Error::Type, "not callable object");
          DISPATCH_ERROR();
        }
        JSFunction* func =
            static_cast<JSFunction*>(callee.object());
        if (func->function_type() == JSFunction::FUNCTION_USER) {
          // inline call
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          Code* code = vm_func->code();
          if (code->empty()) {
            ctx()->RAX() = JSUndefined;
            DISPATCH(EVAL);
          }
          Frame* new_frame = stack_.NewCodeFrame(
              ctx(),
              offset,
              code,
              vm_func->scope(),
              func,
              instr + OPLength<OP::EVAL>::value,
              argc_with_this, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          instr = frame->data();
          strict = frame->code()->strict();
          frame->InitThisBinding(ctx());
          DISPATCH_WITH_NO_INCREMENT();
        }

        // Native Function, so use Invoke
        ctx()->RAX() =
            InvokeMaybeEval(func, offset, argc_with_this, frame, ERR);
        DISPATCH(EVAL);
      }

      DEFINE_OPCODE(RESULT) {
        // opcode | dst
        //
        // Because RESULT opcode is inlined in CALL / EVAL / CONSTRUCT / RETURN,
        // this path is rarely used.
        REG(instr[1].i32[0]) = ctx()->RAX();
        DISPATCH(RESULT);
      }

      DEFINE_OPCODE(PREPARE_DYNAMIC_CALL) {
        // opcode | (dst | basedst | name)
        const Symbol name = frame->GetName(instr[1].ssw.u32);
        if (JSEnv* target_env = GetEnv(frame->lexical_env(), name)) {
          const JSVal res =
              target_env->GetBindingValue(ctx(), name, false, ERR);
          REG(instr[1].ssw.i16[0]) = res;
          REG(instr[1].ssw.i16[1]) = target_env->ImplicitThisValue();
        } else {
          RaiseReferenceError(name, e);
          DISPATCH_ERROR();
        }
        DISPATCH(PREPARE_DYNAMIC_CALL);
      }

      default: {
        std::printf("%s\n", OP::String(instr->u32[0]));
        UNREACHABLE();
      }
    }  // switch

#if !defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
 VM_ERROR:
#endif
    // search exception handler or finally handler.
    // if finally handler found, set value to notify that RETURN_SUBROUTINE
    // should rethrow exception.
    assert(*e);
    frame->MaterializeErrorStack(ctx(), e, instr + 1);
    while (true) {
      bool in_range = false;
      const ExceptionTable& table = frame->code()->exception_table();
      const uint32_t offset = static_cast<uint32_t>(instr - frame->data());
      for (ExceptionTable::const_iterator it = table.begin(),
           last = table.end(); it != last; ++it) {
        const Handler& handler = *it;
        if (in_range && handler.begin() > offset) {
          break;  // handler not found
        }
        if (handler.begin() <= offset && offset < handler.end()) {
          in_range = true;
          switch (handler.type()) {
            case Handler::ITERATOR: {
              // control iterator lifetime
              NativeIterator* it =
                  static_cast<NativeIterator*>(REG(handler.ret()).cell());
              ctx()->ReleaseNativeIterator(it);
              continue;
            }

            case Handler::ENV: {
              // roll up environment
#ifdef DEBUG
              VerifyDynamicEnvironment(frame);
#endif
              frame->set_lexical_env(frame->lexical_env()->outer());
              continue;
            }

            default: {
              // catch or finally
              const JSVal error = e->Detail(ctx());
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
        // because of Frame is code frame,
        // first lexical_env is variable_env.
        // (if Eval / Global, this is not valid)
        assert(frame->lexical_env() == frame->variable_env());
        frame = stack_.Unwind(frame);
        strict = frame->code()->strict();
      }
    }
    break;
  }  // for main loop
  assert(*e);
  return JSEmpty;
#undef INCREMENT_NEXT
#undef DISPATCH_ERROR
#undef DISPATCH
#undef DISPATCH_WITH_NO_INCREMENT
#undef PREDICT
#undef DEFINE_OPCODE
#undef JUMPTO
#undef JUMPBY
#undef REG
#undef ERR
#undef FAST_PATH_LOAD_CONST
#undef FAST_PATH_IF_FALSE
#undef FAST_PATH_BINARY_LT
#undef FAST_PATH_BINARY_ADD
#undef FAST_PATH_JUMP_BY

}  // NOLINT

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_VM_H_
