#ifndef _IV_LV5_RAILGUN_VM_H_
#define _IV_LV5_RAILGUN_VM_H_
#include <cstddef>
#include <vector>
#include <string>
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
#include "lv5/name_iterator.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/code.h"
#include "lv5/railgun/frame.h"
#include "lv5/railgun/jsfunction.h"
#include "lv5/railgun/stack.h"

namespace iv {
namespace lv5 {
namespace railgun {

class JSValRef : public JSVal {
 public:
  friend class VM;

  JSValRef(const JSVal& val)  // NOLINT
    : JSVal(val) { }

  JSValRef(JSVal* val) {  // NOLINT
    value_.struct_.payload_.jsvalref_ = val;
    value_.struct_.tag_ = lv5::detail::kJSValRefTag;
  }

  JSValRef& operator=(const JSVal& rhs) {
    this_type(rhs).swap(*this);
    return *this;
  }

  inline bool IsJSValRef() const {
    return value_.struct_.tag_ == lv5::detail::kJSValRefTag;
  }

  inline bool IsPtr() const {
    return IsJSValRef() || JSVal::IsPtr();
  }

  inline JSVal* Deref() const {
    assert(IsJSValRef());
    return value_.struct_.payload_.jsvalref_;
  }
};

// Global start
std::pair<JSVal, VM::Status> VM::Run(Code* code, Error* e) {
  Frame* frame = stack_.NewGlobalFrame(ctx_, code);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, THROW);
  }
  Instantiate(ctx_, code, frame, false, true, NULL, e);
  if (*e) {
    stack_.Unwind(frame);
    return std::make_pair(JSEmpty, THROW);
  }
  const std::pair<JSVal, Status> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res;
}

// Global
std::pair<JSVal, VM::Status> VM::RunGlobal(Code* code, Error* e) {
  ScopedArguments args(ctx_, 0, e);
  if (*e) {
    return std::make_pair(JSEmpty, THROW);
  }
  args.set_this_binding(ctx_->global_obj());
  Frame* frame = stack_.NewEvalFrame(
      ctx_,
      stack_.GetTop(),
      code,
      ctx_->global_env(),
      ctx_->global_env());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, THROW);
  }
  Instantiate(ctx_, code, frame, false, true, NULL, e);
  if (*e) {
    stack_.Unwind(frame);
    return std::make_pair(JSEmpty, THROW);
  }
  const std::pair<JSVal, Status> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res;
}

// Eval
std::pair<JSVal, VM::Status> VM::RunEval(Code* code,
                                         JSEnv* variable_env,
                                         JSEnv* lexical_env,
                                         JSVal this_binding,
                                         Error* e) {
  ScopedArguments args(ctx_, 0, e);
  if (*e) {
    return std::make_pair(JSEmpty, THROW);
  }
  args.set_this_binding(this_binding);
  Frame* frame = stack_.NewEvalFrame(
      ctx_,
      stack_.GetTop(),
      code,
      variable_env,
      lexical_env);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, THROW);
  }
  Instantiate(ctx_, code, frame, true, false, NULL, e);
  if (*e) {
    stack_.Unwind(frame);
    return std::make_pair(JSEmpty, THROW);
  }
  const std::pair<JSVal, Status> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res;
}

std::pair<JSVal, VM::Status> VM::Execute(const Arguments& args,
                                         JSVMFunction* func, Error* e) {
  Frame* frame = stack_.NewCodeFrame(
      ctx_,
      args.GetEnd(),
      func->code(),
      func->scope(), NULL, args.size(), args.IsConstructorCalled());
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return std::make_pair(JSEmpty, THROW);
  }
  func->InstantiateBindings(ctx_, frame, e);
  if (*e) {
    stack_.Unwind(frame);
    return std::make_pair(JSEmpty, THROW);
  }
  const std::pair<JSVal, Status> res = Execute(frame, e);
  stack_.Unwind(frame);
  return res;
}

std::pair<JSVal, VM::Status> VM::Execute(Frame* start, Error* e) {
  // current frame values
  Frame* frame = start;
  const uint8_t* first_instr = frame->data();
  const uint8_t* instr = first_instr;
  JSVal* sp = frame->stacktop();
  const JSVals* constants = &frame->constants();
  const Code::Names* names = &frame->code()->names();
  bool strict = frame->code()->strict();

#if defined(IV_USE_DIRECT_THREADED_CODE) && defined(IV_COMPILER_GCC)
#define USE_DIRECT_THREADED_CODE
#endif

#ifdef USE_DIRECT_THREADED_CODE

#define DEFINE_OPCODE(op)\
  case OP::op:\
  op:
#define DISPATCH_ERROR() break
#define DISPATCH()\
  do {\
    FETCH();\
    goto *kDispatchTable[opcode];\
  } while (0)
#define PREDICT(op)\
  do {\
    FETCH();\
    assert(opcode == OP::op);\
    goto op;\
  } while (0)

#else

#define DEFINE_OPCODE(op)\
  case OP::op:
#define DISPATCH_ERROR() break
#define DISPATCH() continue
#define PREDICT(op) continue
#endif

#define ERR\
  e);\
  if (*e) {\
    DISPATCH_ERROR();\
  }\
((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define INSTR_OFFSET() reinterpret_cast<uint8_t*>(instr - first_instr)
#define NEXTOP() (*instr++)
#define NEXTARG() (instr += 2, (instr[-1] << 8) + instr[-2])
#define PEEKARG() ((instr[2] << 8) + instr[1])
#define JUMPTO(x) (instr = first_instr + (x))
#define JUMPBY(x) (instr += (x))
#define FETCH()\
  do {\
    opcode = NEXTOP();\
    oparg = (OP::HasArg(opcode)) ?  NEXTARG() : 0;\
  } while (0)

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
  for (uint16_t i = dynamic_env_level_shrink,\
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
  int opcode = 0;
  uint32_t oparg = 0;
  for (;;) {
MAIN_LOOP_START:
    // fetch opcode
#ifdef USE_DIRECT_THREADED_CODE
#define V(label) &&label,
    static const std::array<const void*, OP::NUM_OF_OP + 1> kDispatchTable = { {
      IV_LV5_RAILGUN_OP_LIST(V)
      NULL
    } };
#undef V
    DISPATCH();
#else
    FETCH();
#endif

    // if ok, use DISPATCH.
    // if error, use DISPATCH_ERROR.
    switch (opcode) {
      DEFINE_OPCODE(NOP) {
        DISPATCH();
      }

      DEFINE_OPCODE(NOP_ARGUMENT) {
        DISPATCH();
      }

      // wasted opcodes... (for direct threading)
      DEFINE_OPCODE(HAVE_ARGUMENT)
      DEFINE_OPCODE(HAVE_EX_ARGUMENT) {
        UNREACHABLE();
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
          return std::make_pair(ret, STOP);
        } else {
          frame->ret_ = JSUndefined;
          const JSVal ret =
              (frame->constructor_call_ && !frame->ret_.IsObject()) ?
              frame->GetThis() : frame->ret_;
          // this code is invoked by JS code
          instr = frame->prev_pc_;
          sp = frame->GetPreviousFrameStackTop();
          frame = stack_.Unwind(frame);
          constants = &frame->constants();
          first_instr = frame->data();
          names = &frame->code()->names();
          strict = frame->code()->strict();
          PUSH(ret);
          DISPATCH();
        }
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(LOAD_CONST) {
        const JSVal x = GETITEM(constants, oparg);
        PUSH(x);
        DISPATCH();
      }

      DEFINE_OPCODE(LOAD_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal w = LoadName(frame->lexical_env(), s, strict, ERR);
        PUSH(w);
        DISPATCH();
      }

      DEFINE_OPCODE(LOAD_LOCAL) {
        const JSVal& w = GETLOCAL(oparg);
        PUSH(w);
        DISPATCH();
      }

      DEFINE_OPCODE(LOAD_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal w = LoadName(ctx_->global_env(), s, strict, ERR);
        PUSH(w);
        DISPATCH();
      }

      DEFINE_OPCODE(LOAD_ELEMENT) {
        const JSVal element = POP();
        const JSVal& base = TOP();
        const JSVal res = LoadElement(sp, base, element, strict, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(LOAD_PROP) {
        const JSVal& base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal res = LoadProp(sp, base, s, strict, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal v = TOP();
        StoreName(frame->lexical_env(), s, v, strict, ERR);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_LOCAL) {
        const JSVal v = TOP();
        SETLOCAL(oparg, v);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_LOCAL_IMMUTABLE) {
        assert(!strict);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal v = TOP();
        StoreName(ctx_->global_env(), s, v, strict, ERR);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_ELEMENT) {
        const JSVal w = POP();
        const JSVal element = POP();
        const JSVal base = TOP();
        StoreElement(base, element, w, strict, ERR);
        SET_TOP(w);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_PROP) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal w = POP();
        const JSVal base = TOP();
        StoreProp(base, s, w, strict, ERR);
        SET_TOP(w);
        DISPATCH();
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
        DISPATCH();
      }

      DEFINE_OPCODE(DELETE_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        if (JSEnv* current = GetEnv(frame->lexical_env(), s)) {
          const bool res = current->DeleteBinding(ctx_, s);
          PUSH(JSVal::Bool(res));
        } else {
          // not found -> unresolvable reference
          PUSH(JSTrue);
        }
        DISPATCH();
      }

      DEFINE_OPCODE(DELETE_LOCAL) {
        PUSH(JSFalse);
        DISPATCH();
      }

      DEFINE_OPCODE(DELETE_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        if (JSEnv* current = GetEnv(ctx_->global_env(), s)) {
          const bool res = current->DeleteBinding(ctx_, s);
          PUSH(JSVal::Bool(res));
        } else {
          // not found -> unresolvable reference
          PUSH(JSTrue);
        }
        DISPATCH();
      }

      DEFINE_OPCODE(DELETE_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        uint32_t index;
        if (element.GetUInt32(&index)) {
          JSObject* const obj = base.ToObject(ctx_, ERR);
          const bool result = obj->Delete(ctx_, symbol::MakeSymbolFromIndex(index), strict, ERR);
          SET_TOP(JSVal::Bool(result));
        } else {
          const JSString* str = element.ToString(ctx_, ERR);
          const Symbol s = context::Intern(ctx_, str);
          JSObject* const obj = base.ToObject(ctx_, ERR);
          const bool result = obj->Delete(ctx_, s, strict, ERR);
          SET_TOP(JSVal::Bool(result));
        }
        DISPATCH();
      }

      DEFINE_OPCODE(DELETE_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        base.CheckObjectCoercible(ERR);
        JSObject* const obj = base.ToObject(ctx_, ERR);
        const bool result = obj->Delete(ctx_, s, strict, ERR);
        SET_TOP(JSVal::Bool(result));
        DISPATCH();
      }

      DEFINE_OPCODE(POP_TOP) {
        POP_UNUSED();
        DISPATCH();
      }

      DEFINE_OPCODE(POP_TOP_AND_RET) {
        frame->ret_ = POP();
        DISPATCH();
      }

      DEFINE_OPCODE(POP_N) {
        const int n = -static_cast<int>(oparg);
        STACKADJ(n);
        DISPATCH();
      }

      DEFINE_OPCODE(POP_JUMP_IF_FALSE) {
        const JSVal v = POP();
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPTO(oparg);
        }
        DISPATCH();
      }

      DEFINE_OPCODE(POP_JUMP_IF_TRUE) {
        const JSVal v = POP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPTO(oparg);
        }
        DISPATCH();
      }

      DEFINE_OPCODE(JUMP_IF_TRUE_OR_POP) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPTO(oparg);
        } else {
          POP_UNUSED();
        }
        DISPATCH();
      }

      DEFINE_OPCODE(JUMP_IF_FALSE_OR_POP) {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPTO(oparg);
        } else {
          POP_UNUSED();
        }
        DISPATCH();
      }

      DEFINE_OPCODE(JUMP_SUBROUTINE) {
        const JSVal addr = std::distance(first_instr, instr);
        PUSH(JSEmpty);
        PUSH(addr);
        PUSH(JSVal::Int32(0));
        JUMPTO(oparg);
        DISPATCH();
      }

      DEFINE_OPCODE(JUMP_RETURN_HOOKED_SUBROUTINE) {
        const JSVal addr = std::distance(first_instr, instr);
        PUSH(addr);
        PUSH(JSVal::Int32(1));
        JUMPTO(oparg);
        DISPATCH();
      }

      DEFINE_OPCODE(JUMP_FORWARD) {
        JUMPBY(oparg - 3);
        DISPATCH();
      }

      DEFINE_OPCODE(JUMP_ABSOLUTE) {
        JUMPTO(oparg);
        DISPATCH();
      }

      DEFINE_OPCODE(ROT_TWO) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        SET_TOP(w);
        SET_SECOND(v);
        DISPATCH();
      }

      DEFINE_OPCODE(ROT_THREE) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        const JSVal x = THIRD();
        SET_TOP(w);
        SET_SECOND(x);
        SET_THIRD(v);
        DISPATCH();
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
        DISPATCH();
      }

      DEFINE_OPCODE(DUP_TOP) {
        const JSVal v = TOP();
        PUSH(v);
        DISPATCH();
      }

      DEFINE_OPCODE(DUP_TWO) {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        PUSH(w);
        PUSH(v);
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_NULL) {
        PUSH(JSNull);
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_TRUE) {
        PUSH(JSTrue);
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_FALSE) {
        PUSH(JSFalse);
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_EMPTY) {
        PUSH(JSEmpty);
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_UNDEFINED) {
        PUSH(JSUndefined);
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_THIS) {
        PUSH(frame->GetThis());
        DISPATCH();
      }

      DEFINE_OPCODE(PUSH_ARGUMENTS) {
        const JSVal w = LoadName(
            frame->lexical_env(),
            symbol::arguments(), strict, ERR);
        PUSH(w);
        DISPATCH();
      }

      DEFINE_OPCODE(UNARY_POSITIVE) {
        const JSVal& v = TOP();
        const double x = v.ToNumber(ctx_, ERR);
        SET_TOP(x);
        DISPATCH();
      }

      DEFINE_OPCODE(UNARY_NEGATIVE) {
        const JSVal& v = TOP();
        const double x = v.ToNumber(ctx_, ERR);
        SET_TOP(-x);
        DISPATCH();
      }

      DEFINE_OPCODE(UNARY_NOT) {
        const JSVal& v = TOP();
        const bool x = v.ToBoolean(ERR);
        SET_TOP(JSVal::Bool(!x));
        DISPATCH();
      }

      DEFINE_OPCODE(UNARY_BIT_NOT) {
        const JSVal& v = TOP();
        if (v.IsInt32()) {
          SET_TOP(JSVal::Int32(~v.int32()));
        } else {
          const double value = v.ToNumber(ctx_, ERR);
          SET_TOP(~core::DoubleToInt32(value));
        }
        DISPATCH();
      }

      DEFINE_OPCODE(TYPEOF) {
        const JSVal& v = TOP();
        const JSVal result = v.TypeOf(ctx_);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(TYPEOF_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        if (JSEnv* current = GetEnv(frame->lexical_env(), s)) {
          const JSVal expr = current->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr.TypeOf(ctx_));
        } else {
          // unresolvable reference
          PUSH(JSString::NewAsciiString(ctx_, "undefined"));
        }
        DISPATCH();
      }

      DEFINE_OPCODE(TYPEOF_LOCAL) {
        const JSVal& expr = GETLOCAL(oparg);
        PUSH(expr.TypeOf(ctx_));
        DISPATCH();
      }

      DEFINE_OPCODE(TYPEOF_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        if (JSEnv* current = GetEnv(ctx_->global_env(), s)) {
          const JSVal expr = current->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr.TypeOf(ctx_));
        } else {
          // unresolvable reference
          PUSH(JSString::NewAsciiString(ctx_, "undefined"));
        }
        DISPATCH();
      }

      DEFINE_OPCODE(DECREMENT_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<-1, 1>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<-1, 0>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(INCREMENT_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<1, 1>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<1, 0>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(DECREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev - 1;
        SETLOCAL(oparg, now);
        PUSH(now);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev - 1;
        SETLOCAL(oparg, now);
        PUSH(prev);
        DISPATCH();
      }

      DEFINE_OPCODE(INCREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev + 1;
        SETLOCAL(oparg, now);
        PUSH(now);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_LOCAL) {
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev + 1;
        SETLOCAL(oparg, now);
        PUSH(prev);
        DISPATCH();
      }

      DEFINE_OPCODE(DECREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev - 1;
        PUSH(now);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        PUSH(prev);
        DISPATCH();
      }

      DEFINE_OPCODE(INCREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        const double now = prev + 1;
        PUSH(now);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_LOCAL_IMMUTABLE) {
        assert(!strict);
        const JSVal& w = GETLOCAL(oparg);
        const double prev = w.ToNumber(ctx_, ERR);
        PUSH(prev);
        DISPATCH();
      }

      DEFINE_OPCODE(DECREMENT_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<-1, 1>(ctx_->global_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<-1, 0>(ctx_->global_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(INCREMENT_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<1, 1>(ctx_->global_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementName<1, 0>(ctx_->global_env(), s, strict, ERR);
        PUSH(result);
        DISPATCH();
      }

      DEFINE_OPCODE(DECREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            IncrementElement<-1, 1>(sp, base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            IncrementElement<-1, 0>(sp, base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(INCREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            IncrementElement<1, 1>(sp, base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal result =
            IncrementElement<1, 0>(sp, base, element, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(DECREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementProp<-1, 1>(sp, base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_DECREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementProp<-1, 0>(sp, base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(INCREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementProp<1, 1>(sp, base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH();
      }

      DEFINE_OPCODE(POSTFIX_INCREMENT_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal result = IncrementProp<1, 0>(sp, base, s, strict, ERR);
        SET_TOP(result);
        DISPATCH();
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
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryAdd(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_SUBTRACT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinarySub(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_MULTIPLY) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryMultiply(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_DIVIDE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryDivide(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_MODULO) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryModulo(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_LSHIFT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryLShift(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_RSHIFT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryRShift(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_RSHIFT_LOGICAL) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryRShiftLogical(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_LT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareLT(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_LTE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareLTE(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_GT) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareGT(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_GTE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareGTE(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_INSTANCEOF) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryInstanceof(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_IN) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryIn(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_EQ) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryEqual(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_STRICT_EQ) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryStrictEqual(v, w);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_NE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryNotEqual(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_STRICT_NE) {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryStrictNotEqual(v, w);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_BIT_AND) {  // &
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryBitAnd(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_BIT_XOR) {  // ^
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryBitXor(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(BINARY_BIT_OR) {  // |
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryBitOr(v, w, ERR);
        SET_TOP(res);
        DISPATCH();
      }

      DEFINE_OPCODE(RETURN) {
        frame->ret_ = TOP();
        const JSVal ret =
            (frame->constructor_call_ && !frame->ret_.IsObject()) ?
            frame->GetThis() : frame->ret_;
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          return std::make_pair(ret, RETURN);
        } else {
          // this code is invoked by JS code
          instr = frame->prev_pc_;
          sp = frame->GetPreviousFrameStackTop();
          frame = stack_.Unwind(frame);
          constants = &frame->constants();
          first_instr = frame->data();
          names = &frame->code()->names();
          strict = frame->code()->strict();
          PUSH(ret);
          DISPATCH();
        }
      }

      DEFINE_OPCODE(RETURN_SUBROUTINE) {
        const JSVal flag = POP();
        const JSVal v = POP();
        assert(flag.IsInt32());
        const int32_t f = flag.int32();
        if (f == 0) {
          // JUMP_SUBROUTINE
          // return to caller
          POP_UNUSED();
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          DISPATCH();
        } else if (f == 1) {
          // RETURN HOOK
          assert(v.IsNumber());
          const uint32_t addr = v.GetUInt32();
          JUMPTO(addr);
          DISPATCH();
        } else {
          // ERROR FINALLY JUMP
          // rethrow error
          POP_UNUSED();
          e->Report(v);
          DISPATCH_ERROR();
        }
      }

      DEFINE_OPCODE(SWITCH_CASE) {
        const JSVal v = POP();
        const JSVal w = TOP();
        if (internal::StrictEqual(v, w)) {
          POP_UNUSED();
          JUMPTO(oparg);
        }
        DISPATCH();
      }

      DEFINE_OPCODE(SWITCH_DEFAULT) {
        POP_UNUSED();
        JUMPTO(oparg);
        DISPATCH();
      }

      DEFINE_OPCODE(FORIN_SETUP) {
        const JSVal v = TOP();
        if (v.IsNull() || v.IsUndefined()) {
          // TODO(Constellation) more precise method
          JUMPTO(oparg);  // skip for-in stmt
          DISPATCH();
        }
        POP_UNUSED();
        // TODO(Constellation) implement JSIterator
        JSObject* const obj = v.ToObject(ctx_, ERR);
        NameIterator* it = NameIterator::New(ctx_, obj);
        PUSH(JSVal::Ptr(it));
        PREDICT(FORIN_ENUMERATE);
      }

      DEFINE_OPCODE(FORIN_ENUMERATE) {
        NameIterator* it = reinterpret_cast<NameIterator*>(TOP().pointer());
        if (it->Has()) {
          const Symbol sym = it->Get();
          it->Next();
          PUSH(JSString::New(ctx_, sym));
        } else {
          JUMPTO(oparg);
        }
        DISPATCH();
      }

      DEFINE_OPCODE(THROW) {
        frame->ret_ = POP();
        e->Report(frame->ret_);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(RAISE_IMMUTABLE) {
        const Symbol& s = GETITEM(&frame->code()->locals(), oparg);
        RaiseImmutable(s, e);
        DISPATCH_ERROR();
      }

      DEFINE_OPCODE(DEBUGGER) {
        DISPATCH();
      }

      DEFINE_OPCODE(WITH_SETUP) {
        const JSVal val = POP();
        JSObject* const obj = val.ToObject(ctx_, ERR);
        JSObjectEnv* const with_env =
            internal::NewObjectEnvironment(ctx_, obj, frame->lexical_env());
        with_env->set_provide_this(true);
        frame->set_lexical_env(with_env);
        frame->dynamic_env_level_ += 1;
        DISPATCH();
      }

      DEFINE_OPCODE(POP_ENV) {
        frame->set_lexical_env(frame->lexical_env()->outer());
        frame->dynamic_env_level_ -= 1;
        DISPATCH();
      }

      DEFINE_OPCODE(TRY_CATCH_SETUP) {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal error = POP();
        JSEnv* const catch_env =
            internal::NewStaticEnvironment(ctx_, frame->lexical_env(), s, error);
        frame->set_lexical_env(catch_env);
        frame->dynamic_env_level_ += 1;
        DISPATCH();
      }

      DEFINE_OPCODE(BUILD_ARRAY) {
        JSArray* x = JSArray::ReservedNew(ctx_, oparg);
        PUSH(x);
        DISPATCH();
      }

      DEFINE_OPCODE(INIT_ARRAY_ELEMENT) {
        const JSVal w = POP();
        JSArray* ary = static_cast<JSArray*>(TOP().object());
        ary->Set(oparg, w);
        DISPATCH();
      }

      DEFINE_OPCODE(BUILD_OBJECT) {
        JSObject* x = JSObject::New(ctx_);
        PUSH(x);
        DISPATCH();
      }

      DEFINE_OPCODE(MAKE_CLOSURE) {
        Code* target = frame->code()->codes()[oparg];
        JSFunction* x = JSVMFunction::New(ctx_, target, frame->lexical_env());
        PUSH(x);
        DISPATCH();
      }

      DEFINE_OPCODE(BUILD_REGEXP) {
        const JSVal w = POP();
        JSRegExp* x = JSRegExp::New(ctx_, static_cast<JSRegExp*>(w.object()));
        PUSH(x);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_OBJECT_DATA) {
        const Symbol& name = GETITEM(names, oparg);
        const JSVal value = POP();
        const JSVal obj = TOP();
        obj.object()->DefineOwnProperty(
            ctx_, name,
            DataDescriptor(value,
                           PropertyDescriptor::WRITABLE |
                           PropertyDescriptor::ENUMERABLE |
                           PropertyDescriptor::CONFIGURABLE),
            false, ERR);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_OBJECT_GET) {
        const Symbol& name = GETITEM(names, oparg);
        const JSVal value = POP();
        const JSVal obj = TOP();
        obj.object()->DefineOwnProperty(
            ctx_, name,
            AccessorDescriptor(value.object(), NULL,
                               PropertyDescriptor::ENUMERABLE |
                               PropertyDescriptor::CONFIGURABLE |
                               PropertyDescriptor::UNDEF_SETTER),
            false, ERR);
        DISPATCH();
      }

      DEFINE_OPCODE(STORE_OBJECT_SET) {
        const Symbol& name = GETITEM(names, oparg);
        const JSVal value = POP();
        const JSVal obj = TOP();
        obj.object()->DefineOwnProperty(
            ctx_, name,
            AccessorDescriptor(NULL, value.object(),
                               PropertyDescriptor::ENUMERABLE |
                               PropertyDescriptor::CONFIGURABLE |
                               PropertyDescriptor::UNDEF_GETTER),
            false, ERR);
        DISPATCH();
      }

      DEFINE_OPCODE(CALL) {
        const int argc = oparg;
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
              static_cast<JSVMFunction*>(func)->scope(), instr, argc, false);
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
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_, frame, ERR);
        } else {
          // Native Function, so use Invoke
          const JSVal x = Invoke(func, sp, oparg, e);
          sp -= (argc + 2);
          PUSH(x);
          if (*e) {
            DISPATCH_ERROR();
          }
        }
        DISPATCH();
      }

      DEFINE_OPCODE(CONSTRUCT) {
        const int argc = oparg;
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
              static_cast<JSVMFunction*>(func)->scope(), instr, argc, true);
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
          JSObject* const obj = JSObject::New(ctx_);
          const JSVal proto = func->Get(ctx_, symbol::prototype(), ERR);
          if (proto.IsObject()) {
            obj->set_prototype(proto.object());
          }
          frame->set_this_binding(obj);
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_, frame, ERR);
        } else {
          // Native Function, so use Invoke
          const JSVal x = Construct(func, sp, oparg, e);
          sp -= (argc + 2);
          PUSH(x);
          if (*e) {
            DISPATCH_ERROR();
          }
        }
        DISPATCH();
      }

      DEFINE_OPCODE(EVAL) {
        // maybe direct call to eval
        const int argc = oparg;
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
              static_cast<JSVMFunction*>(func)->scope(), instr, argc, false);
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
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_, frame, ERR);
        } else {
          // Native Function, so use Invoke
          const JSVal x = InvokeMaybeEval(func, sp, oparg, frame, e);
          sp -= (argc + 2);
          PUSH(x);
          if (*e) {
            DISPATCH_ERROR();
          }
        }
        DISPATCH();
      }

      DEFINE_OPCODE(CALL_NAME) {
        const Symbol& s = GETITEM(names, oparg);
        JSVal res;
        if (JSEnv* target_env = GetEnv(frame->lexical_env(), s)) {
          const JSVal w = target_env->GetBindingValue(ctx_, s, false, ERR);
          PUSH(w);
          PUSH(target_env->ImplicitThisValue());
        } else {
          RaiseReferenceError(s, e);
          DISPATCH_ERROR();
        }
        DISPATCH();
      }

      DEFINE_OPCODE(CALL_LOCAL) {
        const JSVal& w = GETLOCAL(oparg);
        PUSH(w);
        PUSH(frame->lexical_env()->ImplicitThisValue());
        DISPATCH();
      }

      DEFINE_OPCODE(CALL_GLOBAL) {
        const Symbol& s = GETITEM(names, oparg);
        JSVal res;
        if (JSEnv* target_env = GetEnv(ctx_->global_env(), s)) {
          const JSVal w = target_env->GetBindingValue(ctx_, s, false, ERR);
          PUSH(w);
          PUSH(target_env->ImplicitThisValue());
        } else {
          RaiseReferenceError(s, e);
          DISPATCH_ERROR();
        }
        DISPATCH();
      }

      DEFINE_OPCODE(CALL_ELEMENT) {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal res = LoadElement(sp, base, element, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        DISPATCH();
      }

      DEFINE_OPCODE(CALL_PROP) {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal res = LoadProp(sp, base, s, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        DISPATCH();
      }

      DEFINE_OPCODE(CALL_CALL_RESULT) {
        PUSH(frame->lexical_env()->ImplicitThisValue());
        DISPATCH();
      }

      default: {
        std::printf("%s\n", OP::String(opcode));
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
        if (begin < offset && offset <= end) {
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
            PUSH(JSVal::Int32(2));
          }
          JUMPTO(end);
          goto MAIN_LOOP_START;
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
  return std::make_pair(JSEmpty, THROW);
#undef DISPATCH_ERROR
#undef DISPATCH
#undef PREDICT
#undef DEFINE_OPCODE
#undef USE_DIRECT_THREADED_CODE
#undef FETCH
#undef NEXTOP
#undef NEXTARG
#undef PEEKARG
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
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_VM_H_
