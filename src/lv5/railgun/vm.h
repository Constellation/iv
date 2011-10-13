#ifndef IV_LV5_RAILGUN_VM_H_
#define IV_LV5_RAILGUN_VM_H_
#include <cstddef>
#include <vector>
#include <string>
#include "notfound.h"
#include "detail/memory.h"
#include "detail/cstdint.h"
#include "detail/tuple.h"
#include "lv5/error_check.h"
#include "lv5/gc_template.h"
#include "lv5/jsval.h"
#include "lv5/jsarray.h"
#include "lv5/jserror.h"
#include "lv5/jsregexp.h"
#include "lv5/property.h"
#include "lv5/internal.h"
#include "lv5/slot.h"
#include "lv5/chain.h"
#include "lv5/name_iterator.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/operation.h"
#include "lv5/railgun/code.h"
#include "lv5/railgun/frame.h"
#include "lv5/railgun/instruction_fwd.h"
#include "lv5/railgun/jsfunction.h"
#include "lv5/railgun/stack.h"
#include "lv5/railgun/direct_threading.h"

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
  Instantiate(ctx_, code, frame, false, true, NULL, e);
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
  Instantiate(ctx_, code, frame, false, true, NULL, e);
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
  Instantiate(ctx_, code, frame, true, false, NULL, e);
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
      func->scope(), NULL, args.size(), args.IsConstructorCalled());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, STATE_THROW);
  }
  func->InstantiateBindings(ctx_, frame, e);
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
  Instruction* first_instr = frame->data();
  Instruction* instr = first_instr;
  JSVal* sp = frame->stacktop();
  const JSVals* constants = &frame->constants();
  const Code::Names* names = &frame->code()->names();
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

#define JUMPTO(x) (instr = first_instr + (x))
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
#define STACKADJ(n) (sp += (n))
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
        constants = &frame->constants();
        first_instr = frame->data();
        names = &frame->code()->names();
        strict = frame->code()->strict();
        PUSH(ret);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(LOAD_CONST) {
        const JSVal x = GETITEM(constants, instr[1].value);
        PUSH(x);
        DISPATCH(LOAD_CONST);
      }

      DEFINE_OPCODE(LOAD_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal w =
            operation_.LoadName(frame->lexical_env(), s, strict, ERR);
        PUSH(w);
        DISPATCH(LOAD_NAME);
      }

      DEFINE_OPCODE(LOAD_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal w =
            operation_.LoadHeap(frame->variable_env(),
                                s, strict, instr[2].value, instr[3].value, ERR);
        PUSH(w);
        DISPATCH(LOAD_HEAP);
      }

      DEFINE_OPCODE(LOAD_LOCAL) {
        const JSVal& w = GETLOCAL(instr[1].value);
        PUSH(w);
        DISPATCH(LOAD_LOCAL);
      }

      DEFINE_OPCODE(LOAD_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const JSVal val =
            operation_.LoadGlobal(global, instr,
                                  GETITEM(names, instr[1].value), strict, ERR);
        PUSH(val);
        DISPATCH(LOAD_GLOBAL);
      }

      DEFINE_OPCODE(LOAD_ELEMENT) {
        const JSVal element = POP();
        const JSVal& base = TOP();
        const JSVal res = operation_.LoadElement(base, element, strict, ERR);
        SET_TOP(res);
        DISPATCH(LOAD_ELEMENT);
      }

      DEFINE_OPCODE(LOAD_PROP) {
        // opcode | name | nop | nop | nop | nop
        const JSVal& base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal res =
            operation_.LoadProp<
              OP::LOAD_PROP_OWN,
              OP::LOAD_PROP_PROTO,
              OP::LOAD_PROP_CHAIN,
              OP::LOAD_PROP_GENERIC>(instr, base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH(LOAD_PROP);
      }

      DEFINE_OPCODE(LOAD_PROP_OWN) {
        // opcode | name | map | offset | nop | nop
        const JSVal& base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          if (operation_.GetPrimitiveOwnProperty(
                  base,
                  GETITEM(names, instr[1].value),
                  &res)) {
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
          const JSVal res = obj->GetSlot(instr[3].value).Get(ctx_, base, ERR);
          SET_TOP(res);
        } else {
          // uncache
          const Symbol& s = GETITEM(names, instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        DISPATCH(LOAD_PROP_OWN);
      }

      DEFINE_OPCODE(LOAD_PROP_PROTO) {
        // opcode | name | map | map | offset | nop
        const JSVal& base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          if (operation_.GetPrimitiveOwnProperty(
                  base,
                  GETITEM(names, instr[1].value),
                  &res)) {
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
          const JSVal res = proto->GetSlot(instr[4].value).Get(ctx_, base, ERR);
          SET_TOP(res);
        } else {
          // uncache
          const Symbol& s = GETITEM(names, instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        DISPATCH(LOAD_PROP_PROTO);
      }

      DEFINE_OPCODE(LOAD_PROP_CHAIN) {
        // opcode | name | chain | map | offset | nop
        const JSVal& base = TOP();
        base.CheckObjectCoercible(ERR);
        JSObject* obj = NULL;
        if (base.IsPrimitive()) {
          // primitive prototype cache
          JSVal res;
          if (operation_.GetPrimitiveOwnProperty(
                  base,
                  GETITEM(names, instr[1].value),
                  &res)) {
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
          const JSVal res =
              cached->GetSlot(instr[4].value).Get(ctx_, base, ERR);
          SET_TOP(res);
        } else {
          // uncache
          const Symbol& s = GETITEM(names, instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::LOAD_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        DISPATCH(LOAD_PROP_CHAIN);
      }

      DEFINE_OPCODE(LOAD_PROP_GENERIC) {
        // no cache
        const JSVal& base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal res = operation_.LoadProp(base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH(LOAD_PROP_GENERIC);
      }

      DEFINE_OPCODE(STORE_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal v = TOP();
        operation_.StoreName(frame->lexical_env(), s, v, strict, ERR);
        DISPATCH(STORE_NAME);
      }

      DEFINE_OPCODE(STORE_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal v = TOP();
        operation_.StoreHeap(frame->variable_env(),
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
          const Symbol& s = GETITEM(names, instr[1].value);
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
        const JSVal w = POP();
        const JSVal element = POP();
        const JSVal base = TOP();
        operation_.StoreElement(base, element, w, strict, ERR);
        SET_TOP(w);
        DISPATCH(STORE_ELEMENT);
      }

      DEFINE_OPCODE(STORE_PROP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal w = POP();
        const JSVal base = TOP();
        operation_.StoreProp(base,
                             instr, OP::STORE_PROP_GENERIC, s, w, strict, ERR);
        SET_TOP(w);
        DISPATCH(STORE_PROP);
      }

      DEFINE_OPCODE(STORE_PROP_GENERIC) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal w = POP();
        const JSVal base = TOP();
        operation_.StoreProp(base, s, w, strict, ERR);
        SET_TOP(w);
        DISPATCH(STORE_PROP_GENERIC);
      }

      DEFINE_OPCODE(STORE_CALL_RESULT) {
        // lv5 reject `func() = 20`
        const JSVal w = POP();
        e->Report(Error::Reference, "target is not reference");
        SET_TOP(w);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(DELETE_CALL_RESULT) {
        // lv5 ignore `delete func()`
        SET_TOP(JSTrue);
        DISPATCH(DELETE_CALL_RESULT);
      }

      DEFINE_OPCODE(DELETE_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
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
        const Symbol& s = GETITEM(names, instr[1].value);
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
        const JSVal element = POP();
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        uint32_t index;
        if (element.GetUInt32(&index)) {
          JSObject* const obj = base.ToObject(ctx_, ERR);
          const bool result =
              obj->Delete(ctx_,
                          symbol::MakeSymbolFromIndex(index), strict, ERR);
          SET_TOP(JSVal::Bool(result));
        } else {
          const JSString* str = element.ToString(ctx_, ERR);
          const Symbol s = context::Intern(ctx_, str);
          JSObject* const obj = base.ToObject(ctx_, ERR);
          const bool result = obj->Delete(ctx_, s, strict, ERR);
          SET_TOP(JSVal::Bool(result));
        }
        DISPATCH(DELETE_ELEMENT);
      }

      DEFINE_OPCODE(DELETE_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        base.CheckObjectCoercible(ERR);
        JSObject* const obj = base.ToObject(ctx_, ERR);
        const bool result = obj->Delete(ctx_, s, strict, ERR);
        SET_TOP(JSVal::Bool(result));
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
        const int n = -static_cast<int>(instr[1].value);
        STACKADJ(n);
        DISPATCH(POP_N);
      }

      DEFINE_OPCODE(POP_JUMP_IF_FALSE) {
        const JSVal v = POP();
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPTO(instr[1].value);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(POP_JUMP_IF_FALSE);
      }

      DEFINE_OPCODE(POP_JUMP_IF_TRUE) {
        const JSVal v = POP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPTO(instr[1].value);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(POP_JUMP_IF_TRUE);
      }

      DEFINE_OPCODE(JUMP_IF_TRUE_OR_POP) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPTO(instr[1].value);
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
          JUMPTO(instr[1].value);
          DISPATCH_WITH_NO_INCREMENT();
        } else {
          POP_UNUSED();
        }
        DISPATCH(JUMP_IF_FALSE_OR_POP);
      }

      DEFINE_OPCODE(JUMP_SUBROUTINE) {
        // calc next address
        const JSVal addr =
            std::distance(first_instr, instr) +
            OPLength<OP::JUMP_SUBROUTINE>::value;
        PUSH(JSEmpty);
        PUSH(addr);
        PUSH(JSVal::Int32(kJumpFromSubroutine));
        JUMPTO(instr[1].value);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_RETURN_HOOKED_SUBROUTINE) {
        // calc next address
        const JSVal addr =
            std::distance(first_instr, instr) +
            OPLength<OP::JUMP_RETURN_HOOKED_SUBROUTINE>::value;
        PUSH(addr);
        PUSH(JSVal::Int32(kJumpFromReturn));
        JUMPTO(instr[1].value);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_FORWARD) {
        JUMPBY(instr[1].value);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(JUMP_ABSOLUTE) {
        JUMPTO(instr[1].value);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(ROT_TWO) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        SET_TOP(w);
        SET_SECOND(v);
        DISPATCH(ROT_TWO);
      }

      DEFINE_OPCODE(ROT_THREE) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        const JSVal x = THIRD();
        SET_TOP(w);
        SET_SECOND(x);
        SET_THIRD(v);
        DISPATCH(ROT_THREE);
      }

      DEFINE_OPCODE(ROT_FOUR) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        const JSVal x = THIRD();
        const JSVal y = FOURTH();
        SET_TOP(w);
        SET_SECOND(x);
        SET_THIRD(y);
        SET_FOURTH(v);
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
        const JSVal result = v.TypeOf(ctx_);
        SET_TOP(result);
        DISPATCH(TYPEOF);
      }

      DEFINE_OPCODE(TYPEOF_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        if (JSEnv* current = operation_.GetEnv(frame->lexical_env(), s)) {
          const JSVal expr = current->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr.TypeOf(ctx_));
        } else {
          // unresolvable reference
          PUSH(JSString::NewAsciiString(ctx_, "undefined"));
        }
        DISPATCH(TYPEOF_NAME);
      }

      DEFINE_OPCODE(TYPEOF_HEAP) {
        JSDeclEnv* decl =
            operation_.GetHeapEnv(frame->variable_env(), instr[2].value);
        assert(decl);
        const JSVal expr = decl->GetByOffset(instr[3].value, strict, ERR);
        PUSH(expr.TypeOf(ctx_));
        DISPATCH(TYPEOF_HEAP);
      }

      DEFINE_OPCODE(TYPEOF_LOCAL) {
        const JSVal& expr = GETLOCAL(instr[1].value);
        PUSH(expr.TypeOf(ctx_));
        DISPATCH(TYPEOF_LOCAL);
      }

      DEFINE_OPCODE(TYPEOF_GLOBAL) {
        const Symbol& s = GETITEM(names, instr[1].value);
        if (ctx_->global_env()->HasBinding(ctx_, s)) {
          const JSVal expr =
              ctx_->global_env()->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr.TypeOf(ctx_));
        } else {
          // unresolvable reference
          PUSH(JSString::NewAsciiString(ctx_, "undefined"));
        }
        DISPATCH(TYPEOF_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementName<-1, 1>(frame->lexical_env(),
                                            s, strict, ERR);
        PUSH(result);
        DISPATCH(DECREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementName<-1, 0>(frame->lexical_env(),
                                            s, strict, ERR);
        PUSH(result);
        DISPATCH(POSTFIX_DECREMENT_NAME);
      }

      DEFINE_OPCODE(INCREMENT_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementName<1, 1>(frame->lexical_env(),
                                           s, strict, ERR);
        PUSH(result);
        DISPATCH(INCREMENT_NAME);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_NAME) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementName<1, 0>(frame->lexical_env(),
                                           s, strict, ERR);
        PUSH(result);
        DISPATCH(POSTFIX_INCREMENT_NAME);
      }

      DEFINE_OPCODE(DECREMENT_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementHeap<-1, 1>(
                frame->variable_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(result);
        DISPATCH(DECREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementHeap<-1, 0>(
                frame->variable_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(result);
        DISPATCH(POSTFIX_DECREMENT_HEAP);
      }

      DEFINE_OPCODE(INCREMENT_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementHeap<1, 1>(
                frame->variable_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(result);
        DISPATCH(INCREMENT_HEAP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementHeap<1, 0>(
                frame->variable_env(), s, strict,
                instr[2].value, instr[3].value, ERR);
        PUSH(result);
        DISPATCH(POSTFIX_INCREMENT_HEAP);
      }

      DEFINE_OPCODE(DECREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev - 1;
        SETLOCAL(instr[1].value, now);
        PUSH(now);
        DISPATCH(DECREMENT_LOCAL);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev - 1;
        SETLOCAL(instr[1].value, now);
        PUSH(prev);
        DISPATCH(POSTFIX_DECREMENT_LOCAL);
      }

      DEFINE_OPCODE(INCREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev + 1;
        SETLOCAL(instr[1].value, now);
        PUSH(now);
        DISPATCH(INCREMENT_LOCAL);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(instr[1].value);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev + 1;
        SETLOCAL(instr[1].value, now);
        PUSH(prev);
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
        const JSVal val =
            operation_.IncrementGlobal<-1, 1>(
                global, instr, GETITEM(names, instr[1].value), strict, ERR);
        PUSH(val);
        DISPATCH(DECREMENT_GLOBAL);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const JSVal val =
            operation_.IncrementGlobal<-1, 0>(
                global, instr, GETITEM(names, instr[1].value), strict, ERR);
        PUSH(val);
        DISPATCH(POSTFIX_DECREMENT_GLOBAL);
      }

      DEFINE_OPCODE(INCREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const JSVal val =
            operation_.IncrementGlobal<1, 1>(
                global, instr, GETITEM(names, instr[1].value), strict, ERR);
        PUSH(val);
        DISPATCH(INCREMENT_GLOBAL);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_GLOBAL) {
        JSGlobal* global = ctx_->global_obj();
        const JSVal val =
            operation_.IncrementGlobal<1, 0>(
                global, instr, GETITEM(names, instr[1].value), strict, ERR);
        PUSH(val);
        DISPATCH(POSTFIX_INCREMENT_GLOBAL);
      }

      DEFINE_OPCODE(DECREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            operation_.IncrementElement<-1, 1>(base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH(DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            operation_.IncrementElement<-1, 0>(base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH(POSTFIX_DECREMENT_ELEMENT);
      }

      DEFINE_OPCODE(INCREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            operation_.IncrementElement<1, 1>(base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH(INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            operation_.IncrementElement<1, 0>(base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH(POSTFIX_INCREMENT_ELEMENT);
      }

      DEFINE_OPCODE(DECREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementProp<-1, 1>(base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH(DECREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementProp<-1, 0>(base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH(POSTFIX_DECREMENT_PROP);
      }

      DEFINE_OPCODE(INCREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementProp<1, 1>(base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH(INCREMENT_PROP);
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal result =
            operation_.IncrementProp<1, 0>(base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH(POSTFIX_INCREMENT_PROP);
      }

      DEFINE_OPCODE(DECREMENT_CALL_RESULT)
      DEFINE_OPCODE(POSTFIX_DECREMENT_CALL_RESULT)
      DEFINE_OPCODE(INCREMENT_CALL_RESULT)
      DEFINE_OPCODE(POSTFIX_INCREMENT_CALL_RESULT) {
        const JSVal base = TOP();
        base.ToNumber(ctx_, ERR);
        e->Report(Error::Reference, "target is not reference");
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(BINARY_ADD) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t sum;
          if (!core::IsAdditionOverflow(lhs.int32(), rhs.int32(), &sum)) {
            SET_TOP(JSVal::Int32(sum));
          } else {
            SET_TOP(JSVal(static_cast<double>(lhs.int32()) +
                          static_cast<double>(rhs.int32())));
          }
        } else {
          const JSVal res = operation_.BinaryAdd(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_ADD);
      }

      DEFINE_OPCODE(BINARY_SUBTRACT) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          int32_t dif;
          if (!core::IsSubtractOverflow(lhs.int32(), rhs.int32(), &dif)) {
            SET_TOP(JSVal::Int32(dif));
          } else {
            SET_TOP(JSVal(static_cast<double>(lhs.int32()) -
                          static_cast<double>(rhs.int32())));
          }
        } else {
          const JSVal res = operation_.BinarySub(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_SUBTRACT);
      }

      DEFINE_OPCODE(BINARY_MULTIPLY) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        // if 1 bit is not found in MSB to MSB - 17 bits,
        // this multiply is safe for overflow
        if (lhs.IsInt32() && rhs.IsInt32() &&
            !((lhs.int32() | rhs.int32()) >> 15)) {
          SET_TOP(JSVal::Int32(lhs.int32() * rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryMultiply(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_MULTIPLY);
      }

      DEFINE_OPCODE(BINARY_DIVIDE) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        const JSVal res = operation_.BinaryDivide(lhs, rhs, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_DIVIDE);
      }

      DEFINE_OPCODE(BINARY_MODULO) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        // check rhs is more than 0 (n % 0 == NaN)
        // lhs is >= 0 and rhs is > 0 because example like
        //   -1 % -1
        // should return -0.0, so this value is double
        if (lhs.IsInt32() && rhs.IsInt32() &&
            lhs.int32() >= 0 && rhs.int32() > 0) {
          SET_TOP(JSVal::Int32(lhs.int32() % rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryModulo(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_MODULO);
      }

      DEFINE_OPCODE(BINARY_LSHIFT) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Int32(lhs.int32() << (rhs.int32() & 0x1f)));
        } else {
          const JSVal res = operation_.BinaryLShift(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_LSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Int32(lhs.int32() >> (rhs.int32() & 0x1f)));
        } else {
          const JSVal res = operation_.BinaryRShift(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_RSHIFT);
      }

      DEFINE_OPCODE(BINARY_RSHIFT_LOGICAL) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        uint32_t left_result;
        if (lhs.GetUInt32(&left_result) && rhs.IsInt32()) {
          SET_TOP(JSVal::UInt32(left_result >> (rhs.int32() & 0x1f)));
        } else {
          const JSVal res = operation_.BinaryRShiftLogical(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_RSHIFT_LOGICAL);
      }

      DEFINE_OPCODE(BINARY_LT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = operation_.BinaryCompareLT(v, w, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_LT);
      }

      DEFINE_OPCODE(BINARY_LTE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = operation_.BinaryCompareLTE(v, w, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_LTE);
      }

      DEFINE_OPCODE(BINARY_GT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = operation_.BinaryCompareGT(v, w, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_GT);
      }

      DEFINE_OPCODE(BINARY_GTE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = operation_.BinaryCompareGTE(v, w, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_GTE);
      }

      DEFINE_OPCODE(BINARY_INSTANCEOF) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = operation_.BinaryInstanceof(v, w, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_INSTANCEOF);
      }

      DEFINE_OPCODE(BINARY_IN) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = operation_.BinaryIn(v, w, ERR);
        SET_TOP(res);
        DISPATCH(BINARY_IN);
      }

      DEFINE_OPCODE(BINARY_EQ) {
        const JSVal rhs = POP();
        const JSVal& lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Bool(lhs.int32() == rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryEqual(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_EQ);
      }

      DEFINE_OPCODE(BINARY_STRICT_EQ) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Bool(lhs.int32() == rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryStrictEqual(lhs, rhs);
          SET_TOP(res);
        }
        DISPATCH(BINARY_STRICT_EQ);
      }

      DEFINE_OPCODE(BINARY_NE) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Bool(lhs.int32() != rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryNotEqual(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_NE);
      }

      DEFINE_OPCODE(BINARY_STRICT_NE) {
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Bool(lhs.int32() != rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryStrictNotEqual(lhs, rhs);
          SET_TOP(res);
        }
        DISPATCH(BINARY_STRICT_NE);
      }

      DEFINE_OPCODE(BINARY_BIT_AND) {  // &
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Int32(lhs.int32() & rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryBitAnd(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_BIT_AND);
      }

      DEFINE_OPCODE(BINARY_BIT_XOR) {  // ^
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Int32(lhs.int32() ^ rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryBitXor(lhs, rhs, ERR);
          SET_TOP(res);
        }
        DISPATCH(BINARY_BIT_XOR);
      }

      DEFINE_OPCODE(BINARY_BIT_OR) {  // |
        const JSVal rhs = POP();
        const JSVal lhs = TOP();
        if (lhs.IsInt32() && rhs.IsInt32()) {
          SET_TOP(JSVal::Int32(lhs.int32() | rhs.int32()));
        } else {
          const JSVal res = operation_.BinaryBitOr(lhs, rhs, ERR);
          SET_TOP(res);
        }
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
        constants = &frame->constants();
        first_instr = frame->data();
        names = &frame->code()->names();
        strict = frame->code()->strict();
        PUSH(ret);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(RETURN_SUBROUTINE) {
        const JSVal flag = POP();
        const JSVal v = POP();
        assert(flag.IsInt32());
        const int32_t f = flag.int32();
        if (f == kJumpFromSubroutine) {
          // JUMP_SUBROUTINE
          // return to caller
          POP_UNUSED();
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          DISPATCH_WITH_NO_INCREMENT();
        } else if (f == kJumpFromReturn) {
          // RETURN HOOK
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          DISPATCH_WITH_NO_INCREMENT();
        } else {
          // ERROR FINALLY JUMP
          // rethrow error
          assert(f == kJumpFromFinally);
          POP_UNUSED();
          e->Report(v);
          DISPATCH_ERROR();
        }
      }

      DEFINE_OPCODE(SWITCH_CASE) {
        const JSVal v = POP();
        const JSVal w = TOP();
        if (JSVal::StrictEqual(v, w)) {
          POP_UNUSED();
          JUMPTO(instr[1].value);
          DISPATCH_WITH_NO_INCREMENT();
        }
        DISPATCH(SWITCH_CASE);
      }

      DEFINE_OPCODE(SWITCH_DEFAULT) {
        POP_UNUSED();
        JUMPTO(instr[1].value);
        DISPATCH_WITH_NO_INCREMENT();
      }

      DEFINE_OPCODE(FORIN_SETUP) {
        const JSVal v = TOP();
        if (v.IsNull() || v.IsUndefined()) {
          JUMPTO(instr[1].value);  // skip for-in stmt
          DISPATCH_WITH_NO_INCREMENT();
        }
        POP_UNUSED();
        JSObject* const obj = v.ToObject(ctx_, ERR);
        NameIterator* it = NameIterator::New(ctx_, obj);
        PUSH(JSVal::Cell(it));
        PREDICT(FORIN_SETUP, FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(FORIN_ENUMERATE) {
        NameIterator* it = reinterpret_cast<NameIterator*>(TOP().cell());
        if (it->Has()) {
          const Symbol sym = it->Get();
          it->Next();
          PUSH(JSString::New(ctx_, sym));
        } else {
          JUMPTO(instr[1].value);
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
        const Symbol& s = GETITEM(&frame->code()->locals(), instr[1].value);
        operation_.RaiseImmutable(s, e);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(DEBUGGER) {
        DISPATCH(DEBUGGER);
      }

      DEFINE_OPCODE(WITH_SETUP) {
        const JSVal val = POP();
        JSObject* const obj = val.ToObject(ctx_, ERR);
        JSObjectEnv* const with_env =
            JSObjectEnv::New(ctx_, frame->lexical_env(), obj);
        with_env->set_provide_this(true);
        frame->set_lexical_env(with_env);
        frame->dynamic_env_level_ += 1;
        DISPATCH(WITH_SETUP);
      }

      DEFINE_OPCODE(POP_ENV) {
        frame->set_lexical_env(frame->lexical_env()->outer());
        frame->dynamic_env_level_ -= 1;
        DISPATCH(POP_ENV);
      }

      DEFINE_OPCODE(TRY_CATCH_SETUP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal error = POP();
        JSEnv* const catch_env =
            JSStaticEnv::New(ctx_, frame->lexical_env(), s, error);
        frame->set_lexical_env(catch_env);
        frame->dynamic_env_level_ += 1;
        DISPATCH(TRY_CATCH_SETUP);
      }

      DEFINE_OPCODE(BUILD_ARRAY) {
        JSArray* x = JSArray::ReservedNew(ctx_, instr[1].value);
        PUSH(x);
        DISPATCH(BUILD_ARRAY);
      }

      DEFINE_OPCODE(INIT_VECTOR_ARRAY_ELEMENT) {
        const JSVal w = POP();
        JSArray* ary = static_cast<JSArray*>(TOP().object());
        ary->SetToVector(instr[1].value, w);
        DISPATCH(INIT_VECTOR_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(INIT_SPARSE_ARRAY_ELEMENT) {
        const JSVal w = POP();
        JSArray* ary = static_cast<JSArray*>(TOP().object());
        ary->SetToMap(instr[1].value, w);
        DISPATCH(INIT_SPARSE_ARRAY_ELEMENT);
      }

      DEFINE_OPCODE(BUILD_OBJECT) {
        JSObject* x = JSObject::New(ctx_, instr[1].map);
        PUSH(x);
        DISPATCH(BUILD_OBJECT);
      }

      DEFINE_OPCODE(BUILD_FUNCTION) {
        Code* target = frame->code()->codes()[instr[1].value];
        JSFunction* x = JSVMFunction::New(ctx_, target, frame->lexical_env());
        PUSH(x);
        DISPATCH(BUILD_FUNCTION);
      }

      DEFINE_OPCODE(BUILD_REGEXP) {
        const JSVal w = POP();
        JSRegExp* x = JSRegExp::New(ctx_, static_cast<JSRegExp*>(w.object()));
        PUSH(x);
        DISPATCH(BUILD_REGEXP);
      }

      DEFINE_OPCODE(STORE_OBJECT_DATA) {
        // opcode | offset | merged
        const JSVal value = POP();
        const JSVal obj = TOP();
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
        DISPATCH(STORE_OBJECT_DATA);
      }

      DEFINE_OPCODE(STORE_OBJECT_GET) {
        // opcode | offset | merged
        const JSVal value = POP();
        const JSVal obj = TOP();
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
        DISPATCH(STORE_OBJECT_GET);
      }

      DEFINE_OPCODE(STORE_OBJECT_SET) {
        // opcode | offset | merged
        const JSVal value = POP();
        const JSVal obj = TOP();
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
              instr, argc, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          first_instr = frame->data();
          instr = first_instr;
          sp = frame->stacktop();
          constants = &frame->constants();
          names = &frame->code()->names();
          strict = frame->code()->strict();
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_,
                                                                frame, ERR);
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
              instr, argc, true);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          first_instr = frame->data();
          instr = first_instr;
          sp = frame->stacktop();
          constants = &code->constants();
          names = &code->names();
          strict = code->strict();
          JSObject* const obj = JSObject::New(ctx_, code->ConstructMap(ctx_));
          const JSVal proto = func->Get(ctx_, symbol::prototype(), ERR);
          if (proto.IsObject()) {
            obj->set_prototype(proto.object());
          }
          frame->set_this_binding(obj);
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_,
                                                                frame, ERR);
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
              instr, argc, false);
          if (!new_frame) {
            e->Report(Error::Range, "maximum call stack size exceeded");
            DISPATCH_ERROR();
          }
          frame = new_frame;
          first_instr = frame->data();
          instr = first_instr;
          sp = frame->stacktop();
          constants = &frame->constants();
          names = &frame->code()->names();
          strict = frame->code()->strict();
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_,
                                                                frame, ERR);
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
        const Symbol& s = GETITEM(names, instr[1].value);
        if (JSEnv* target_env = operation_.GetEnv(frame->lexical_env(), s)) {
          const JSVal w = target_env->GetBindingValue(ctx_, s, false, ERR);
          PUSH(w);
          PUSH(target_env->ImplicitThisValue());
        } else {
          operation_.RaiseReferenceError(s, e);
          DISPATCH_ERROR();
        }
        DISPATCH(CALL_NAME);
      }

      DEFINE_OPCODE(CALL_HEAP) {
        const Symbol& s = GETITEM(names, instr[1].value);
        const JSVal w =
            operation_.LoadHeap(frame->variable_env(), s, strict,
                                instr[2].value, instr[3].value, ERR);
        PUSH(w);
        PUSH(JSUndefined);
        DISPATCH(CALL_HEAP);
      }

      DEFINE_OPCODE(CALL_LOCAL) {
        const JSVal& w = GETLOCAL(instr[1].value);
        PUSH(w);
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
          const Symbol& s = GETITEM(names, instr[1].value);
          Slot slot;
          if (global->GetOwnPropertySlot(ctx_, s, &slot)) {
            if (slot.IsCacheable()) {
              instr[2].map = global->map();
              instr[3].value = slot.offset();
            } else {
              // not implemented yet
              UNREACHABLE();
            }
            const JSVal val = slot.Get(ctx_, global, ERR);
            PUSH(val);
          } else {
            const JSVal w =
                operation_.LoadName(ctx_->global_env(), s, strict, ERR);
            PUSH(w);
          }
        }
        PUSH(JSUndefined);
        DISPATCH(CALL_GLOBAL);
      }

      DEFINE_OPCODE(CALL_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal res = operation_.LoadElement(base, element, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        DISPATCH(CALL_ELEMENT);
      }

      DEFINE_OPCODE(CALL_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
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
          if (operation_.GetPrimitiveOwnProperty(
                  base,
                  GETITEM(names, instr[1].value),
                  &res)) {
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
          const Symbol& s = GETITEM(names, instr[1].value);
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
          if (operation_.GetPrimitiveOwnProperty(
                  base,
                  GETITEM(names, instr[1].value),
                  &res)) {
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
          const Symbol& s = GETITEM(names, instr[1].value);
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
          if (operation_.GetPrimitiveOwnProperty(
                  base,
                  GETITEM(names, instr[1].value),
                  &res)) {
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
          const Symbol& s = GETITEM(names, instr[1].value);
          instr[0] = Instruction::GetOPInstruction(OP::CALL_PROP);
          const JSVal res = operation_.LoadProp(base, s, strict, ERR);
          SET_TOP(res);
        }
        PUSH(base);
        DISPATCH(CALL_PROP_CHAIN);
      }

      DEFINE_OPCODE(CALL_PROP_GENERIC) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, instr[1].value);
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
    typedef Code::ExceptionTable ExceptionTable;
    while (true) {
      const ExceptionTable& table = frame->code()->exception_table();
      for (ExceptionTable::const_iterator it = table.begin(),
           last = table.end(); it != last; ++it) {
        const int handler = std::get<0>(*it);
        const uint16_t begin = std::get<1>(*it);
        const uint16_t end = std::get<2>(*it);
        const uint16_t stack_base_level = std::get<3>(*it);
        const uint16_t env_level = std::get<4>(*it);
        const uint32_t offset = static_cast<uint32_t>(instr - first_instr);
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
        constants = &frame->constants();
        first_instr = frame->data();
        names = &frame->code()->names();
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
