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
#include "lv5/stack_resource.h"
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
    value_.struct_.tag_ = detail::kJSValRefTag;
  }

  JSValRef& operator=(const JSVal& rhs) {
    this_type(rhs).swap(*this);
    return *this;
  }

  inline bool IsJSValRef() const {
    return value_.struct_.tag_ == detail::kJSValRefTag;
  }

  inline bool IsPtr() const {
    return IsJSValRef() || JSVal::IsPtr();
  }

  inline JSVal* Deref() const {
    assert(IsJSValRef());
    return value_.struct_.payload_.jsvalref_;
  }
};

int VM::Run(Context* ctx, Code* code) {
  ctx_ = ctx;
  Frame* frame = stack_.NewGlobalFrame(ctx, code);
  Execute(frame);
  stack_.Unwind(frame);
  ctx_ = NULL;
  return EXIT_SUCCESS;
}

std::pair<JSVal, VM::Status> VM::Execute(const Arguments& args, JSVMFunction* func) {
  Frame* frame = stack_.NewCodeFrame(ctx_, func->code(), func->scope(), NULL, args.size());
  const std::pair<JSVal, VM::Status> res = Execute(frame);
  stack_.Unwind(frame);
  return res;
}

std::pair<JSVal, VM::Status> VM::Execute(Frame* start) {
  // current frame values
  Frame* frame = start;
  const Code* code = frame->code();
  const uint8_t* first_instr = frame->data();
  const uint8_t* instr = first_instr;
  JSVal* sp = frame->stacktop();
  uint32_t dynamic_env_level = 0;
  const JSVals* constants = &frame->constants();
  const Code::Names* names = &code->names();
  bool strict = code->strict();

#define ERR\
  &e);\
  if (e) {\
    break;\
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
#define PUSH(x) (*sp++ = (x))
#define POP() (*--sp)
#define POP_UNUSED() (--sp)
#define STACKADJ(n) (sp += (n))
#define UNWIND_STACK(n) (sp = (frame->stacktop() + (n)))
#define UNWIND_DYNAMIC_ENV(n)\
do {\
  const uint16_t dynamic_env_level_shrink = (n);\
  JSEnv* dynamic_env_target = frame->lexical_env();\
  assert(dynamic_env_level >= dynamic_env_level_shrink);\
  for (uint16_t i = dynamic_env_level_shrink; i < dynamic_env_level; ++i) {\
    dynamic_env_target = dynamic_env_target->outer();\
  }\
  frame->set_lexical_env(dynamic_env_target);\
  dynamic_env_level = dynamic_env_level_shrink;\
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

#define GETLOCAL(i) (fast_locals[(i)])
#define SETLOCAL(i, v) do {\
(fast_locals[(i)]) = (v);\
} while (0)

#define GETITEM(target, i) ((*target)[(i)])

  // error object
  Error e;

  // main loop
  for (;;) {
    // fetch opcode
    const int opcode = NEXTOP();
    const uint32_t oparg = (OP::HasArg(opcode)) ?  NEXTARG() : 0;

    // if ok, use continue.
    // if error, use break.
    switch (opcode) {
      case OP::STOP_CODE: {
        break;
      }

      case OP::NOP: {
        continue;
      }

      case OP::LOAD_CONST: {
        const JSVal x = GETITEM(constants, oparg);
        PUSH(x);
        continue;
      }

      case OP::LOAD_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal w = LoadName(frame->lexical_env(), s, strict, ERR);
        PUSH(w);
        continue;
      }

      case OP::LOAD_ELEMENT: {
        const JSVal element = POP();
        const JSVal& base = TOP();
        const JSVal res = LoadElement(sp, base, element, strict, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::LOAD_PROP: {
        const JSVal& base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal res = LoadProp(sp, base, s, strict, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::STORE_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal v = TOP();
        StoreName(frame->lexical_env(), s, v, strict, ERR);
        continue;
      }

      case OP::STORE_ELEMENT: {
        const JSVal w = POP();
        const JSVal element = POP();
        const JSVal base = TOP();
        StoreElement(base, element, w, strict, ERR);
        SET_TOP(w);
        continue;
      }

      case OP::STORE_PROP: {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal w = POP();
        const JSVal base = TOP();
        StoreProp(base, s, w, strict, ERR);
        SET_TOP(w);
        continue;
      }

      case OP::STORE_CALL_RESULT: {
        // lv5 reject `func() = 20`
        const JSVal w = POP();
        e.Report(Error::Reference, "target is not reference");
        SET_TOP(w);
        break;
      }

      case OP::DELETE_CALL_RESULT: {
        // lv5 ignore `delete func()`
        SET_TOP(JSTrue);
        continue;
      }

      case OP::DELETE_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        if (JSEnv* current = GetEnv(frame->lexical_env(), s)) {
          const bool res = current->DeleteBinding(ctx_, s);
          PUSH(JSVal::Bool(res));
        } else {
          // not found -> unresolvable reference
          PUSH(JSTrue);
        }
        continue;
      }

      case OP::DELETE_ELEMENT: {
        const JSVal element = POP();
        const JSVal base = TOP();
        base.CheckObjectCoercible(ERR);
        const JSString* str = element.ToString(ctx_, ERR);
        const Symbol s = context::Intern(ctx_, str->value());
        JSObject* const obj = base.ToObject(ctx_, ERR);
        const bool result = obj->Delete(ctx_, s, strict, ERR);
        SET_TOP(JSVal::Bool(result));
        continue;
      }

      case OP::DELETE_PROP: {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        base.CheckObjectCoercible(ERR);
        JSObject* const obj = base.ToObject(ctx_, ERR);
        const bool result = obj->Delete(ctx_, s, strict, ERR);
        SET_TOP(JSVal::Bool(result));
        continue;
      }

      case OP::POP_TOP: {
        POP_UNUSED();
        continue;
      }

      case OP::POP_TOP_AND_RET: {
        frame->ret_ = POP();
        continue;
      }

      case OP::POP_JUMP_IF_FALSE: {
        const JSVal v = POP();
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPTO(oparg);
        }
        continue;
      }

      case OP::POP_JUMP_IF_TRUE: {
        const JSVal v = POP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPTO(oparg);
        }
        continue;
      }

      case OP::JUMP_IF_TRUE_OR_POP: {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (x) {
          JUMPTO(oparg);
        } else {
          POP_UNUSED();
        }
        continue;
      }

      case OP::JUMP_IF_FALSE_OR_POP: {
        const JSVal v = TOP();
        const bool x = v.ToBoolean(ERR);
        if (!x) {
          JUMPTO(oparg);
        } else {
          POP_UNUSED();
        }
        continue;
      }

      case OP::JUMP_SUBROUTINE: {
        const JSVal addr = JSVal::UInt32(
            static_cast<uint32_t>(std::distance(first_instr, instr)));
        PUSH(addr);
        PUSH(JSTrue);
        JUMPTO(oparg);
        continue;
      }

      case OP::JUMP_FORWARD: {
        JUMPBY(oparg - 3);
        continue;
      }

      case OP::JUMP_ABSOLUTE: {
        JUMPTO(oparg);
        continue;
      }

      case OP::ROT_TWO: {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        SET_TOP(w);
        SET_SECOND(v);
        continue;
      }

      case OP::ROT_THREE: {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        const JSVal x = THIRD();
        SET_TOP(w);
        SET_SECOND(x);
        SET_THIRD(v);
        continue;
      }

      case OP::ROT_FOUR: {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        const JSVal x = THIRD();
        const JSVal y = FOURTH();
        SET_TOP(w);
        SET_SECOND(x);
        SET_THIRD(y);
        SET_FOURTH(v);
        continue;
      }

      case OP::DUP_TOP: {
        const JSVal v = TOP();
        PUSH(v);
        continue;
      }

      case OP::DUP_TWO: {
        const JSVal v = TOP();
        const JSVal w = SECOND();
        PUSH(w);
        PUSH(v);
        continue;
      }

      case OP::PUSH_NULL: {
        PUSH(JSNull);
        continue;
      }

      case OP::PUSH_TRUE: {
        PUSH(JSTrue);
        continue;
      }

      case OP::PUSH_FALSE: {
        PUSH(JSFalse);
        continue;
      }

      case OP::PUSH_EMPTY: {
        PUSH(JSEmpty);
        continue;
      }

      case OP::PUSH_UNDEFINED: {
        PUSH(JSUndefined);
        continue;
      }

      case OP::PUSH_THIS: {
        PUSH(frame->GetThis());
        continue;
      }

      case OP::UNARY_POSITIVE: {
        const JSVal& v = TOP();
        const double x = v.ToNumber(ctx_, ERR);
        SET_TOP(x);
        continue;
      }

      case OP::UNARY_NEGATIVE: {
        const JSVal& v = TOP();
        const double x = v.ToNumber(ctx_, ERR);
        SET_TOP(-x);
        continue;
      }

      case OP::UNARY_NOT: {
        const JSVal& v = TOP();
        const bool x = v.ToBoolean(ERR);
        SET_TOP(JSVal::Bool(!x));
        continue;
      }

      case OP::UNARY_BIT_NOT: {
        const JSVal& v = TOP();
        const double value = v.ToNumber(ctx_, ERR);
        SET_TOP(~core::DoubleToInt32(value));
        continue;
      }

      case OP::TYPEOF: {
        const JSVal& v = TOP();
        const JSVal result = v.TypeOf(ctx_);
        SET_TOP(result);
        continue;
      }

      case OP::TYPEOF_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        if (JSEnv* current = GetEnv(frame->lexical_env(), s)) {
          const JSVal expr = current->GetBindingValue(ctx_, s, strict, ERR);
          PUSH(expr.TypeOf(ctx_));
        } else {
          // unresolvable reference
          PUSH(JSString::NewAsciiString(ctx_, "undefined"));
        }
        continue;
      }

      case OP::DECREMENT_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementName<-1, 1>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        continue;
      }

      case OP::POSTFIX_DECREMENT_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementName<-1, 0>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        continue;
      }

      case OP::INCREMENT_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementName<1, 1>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        continue;
      }

      case OP::POSTFIX_INCREMENT_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementName<1, 0>(frame->lexical_env(), s, strict, ERR);
        PUSH(result);
        continue;
      }

      case OP::DECREMENT_ELEMENT: {
        const JSVal element = POP();
        const JSVal base = TOP();
        const double result =
            IncrementElement<-1, 1>(sp, base, element, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::POSTFIX_DECREMENT_ELEMENT: {
        const JSVal element = POP();
        const JSVal base = TOP();
        const double result =
            IncrementElement<-1, 0>(sp, base, element, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::INCREMENT_ELEMENT: {
        const JSVal element = POP();
        const JSVal base = TOP();
        const double result =
            IncrementElement<1, 1>(sp, base, element, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::POSTFIX_INCREMENT_ELEMENT: {
        const JSVal element = POP();
        const JSVal base = TOP();
        const double result =
            IncrementElement<1, 0>(sp, base, element, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::DECREMENT_PROP: {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementProp<-1, 1>(sp, base, s, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::POSTFIX_DECREMENT_PROP: {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementProp<-1, 0>(sp, base, s, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::INCREMENT_PROP: {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementProp<1, 1>(sp, base, s, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::POSTFIX_INCREMENT_PROP: {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const double result = IncrementProp<1, 0>(sp, base, s, strict, ERR);
        SET_TOP(result);
        continue;
      }

      case OP::BINARY_ADD: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryAdd(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_SUBTRACT: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinarySub(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_MULTIPLY: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryMultiply(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_DIVIDE: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryDivide(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_MODULO: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryModulo(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_LSHIFT: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryLShift(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_RSHIFT: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryRShift(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_RSHIFT_LOGICAL: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryRShiftLogical(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_LT: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareLT(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_LTE: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareLTE(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_GT: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareGT(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_GTE: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryCompareGTE(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_INSTANCEOF: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryInstanceof(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_IN: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryIn(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_EQ: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryEqual(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_STRICT_EQ: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryStrictEqual(v, w);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_NE: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryNotEqual(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_STRICT_NE: {
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryStrictNotEqual(v, w);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_BIT_AND: {  // &
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryBitAnd(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_BIT_XOR: {  // ^
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryBitXor(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::BINARY_BIT_OR: {  // |
        const JSVal w = POP();
        const JSVal& v = TOP();
        const JSVal res = BinaryBitOr(v, w, ERR);
        SET_TOP(res);
        continue;
      }

      case OP::RETURN: {
        frame->ret_ = TOP();
        // if previous code is not native code, unwind frame and jump
        if (frame->prev_pc_ == NULL) {
          // this code is invoked by native function
          return std::make_pair(frame->ret_, RETURN);
        } else {
          // this code is invoked by JS code
          code = frame->prev_->code();
          first_instr = frame->prev_->data();
          instr = frame->prev_pc_;
          sp = frame->GetPreviousFrameStackTop();
          // TODO(Constellation) fix this value
          dynamic_env_level = 0;
          constants = &frame->prev_->constants();
          names = &code->names();
          strict = code->strict();
          const JSVal ret = frame->ret_;
          frame = stack_.Unwind(frame);
          PUSH(ret);
          continue;
        }
      }

      case OP::SET_RET_VALUE: {
        frame->ret_ = POP();
        continue;
      }

      case OP::RETURN_RET_VALUE: {
        return std::make_pair(frame->ret_, RETURN);
      }

      case OP::RETURN_SUBROUTINE: {
        const JSVal flag = POP();
        const JSVal v = POP();
        assert(flag.IsBoolean());
        if (flag.boolean()) {
          // JUMP_SUBROUTINE
          // return to caller
          // v is always UInt32
          const uint32_t addr = v.uint32();
          JUMPTO(addr);
          continue;
        } else {
          // ERROR FINALLY JUMP
          // rethrow error
          e.Report(v);
          break;
        }
      }

      case OP::THROW: {
        frame->ret_ = POP();
        e.Report(frame->ret_);
        break;
      }

      case OP::WITH_SETUP: {
        const JSVal val = POP();
        JSObject* const obj = val.ToObject(ctx_, ERR);
        JSObjectEnv* const with_env =
            internal::NewObjectEnvironment(ctx_, obj, frame->lexical_env());
        with_env->set_provide_this(true);
        frame->set_lexical_env(with_env);
        ++dynamic_env_level;
        continue;
      }

      case OP::POP_ENV: {
        frame->set_lexical_env(frame->lexical_env()->outer());
        --dynamic_env_level;
        continue;
      }

      case OP::TRY_CATCH_SETUP: {
        const Symbol& s = GETITEM(names, oparg);
        const JSVal error = POP();
        JSEnv* const catch_env =
            internal::NewDeclarativeEnvironment(ctx_, frame->lexical_env());
        catch_env->CreateMutableBinding(ctx_, s, false, ERR);
        catch_env->SetMutableBinding(ctx_, s, error, false, ERR);
        frame->set_lexical_env(catch_env);
        ++dynamic_env_level;
        continue;
      }

      case OP::BUILD_ARRAY: {
        JSArray* x = JSArray::ReservedNew(ctx_, oparg);
        PUSH(x);
        continue;
      }

      case OP::INIT_ARRAY_ELEMENT: {
        const JSVal w = POP();
        JSArray* ary = static_cast<JSArray*>(TOP().object());
        ary->Set(oparg, w);
        continue;
      }

      case OP::BUILD_OBJECT: {
        JSObject* x = JSObject::New(ctx_);
        PUSH(x);
        continue;
      }

      case OP::MAKE_CLOSURE: {
        Code* target = code->codes()[oparg];
        JSFunction* x = JSVMFunction::New(ctx_, target, frame->lexical_env());
        PUSH(x);
        continue;
      }

      case OP::BUILD_REGEXP: {
        const JSVal w = POP();
        JSRegExp* x = JSRegExp::New(ctx_, static_cast<JSRegExp*>(w.object()));
        PUSH(x);
        continue;
      }

      case OP::STORE_OBJECT_DATA: {
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
        continue;
      }

      case OP::STORE_OBJECT_GET: {
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
        continue;
      }

      case OP::STORE_OBJECT_SET: {
        const Symbol& name = GETITEM(names, oparg);
        const JSVal value = POP();
        const JSVal obj = TOP();
        obj.object()->DefineOwnProperty(
            ctx_, name,
            AccessorDescriptor(NULL, value.object(),
                               PropertyDescriptor::ENUMERABLE |
                               PropertyDescriptor::CONFIGURABLE |
                               PropertyDescriptor::UNDEF_SETTER),
            false, ERR);
        continue;
      }

      case OP::CALL: {
        const int argc = oparg;
        const JSVal v = sp[-(argc + 2)];
        if (!v.IsCallable()) {
          e.Report(Error::Type, "not callable object");
          break;
        }
        JSFunction* func = v.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          frame = stack_.NewCodeFrame(
              ctx_,
              static_cast<JSVMFunction*>(func)->code(),
              static_cast<JSVMFunction*>(func)->scope(), instr, argc);
          if (!frame) {
            e.Report(Error::Range, "maximum call stack size exceeded");
            break;
          }
          code = frame->code();
          first_instr = frame->data();
          instr = first_instr;
          sp = frame->stacktop();
          dynamic_env_level = 0;
          constants = &frame->constants();
          names = &code->names();
          strict = code->strict();
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_, frame, ERR);
        } else {
          // Native Function, so use Invoke
          JSVal* stack_pointer = sp;
          const JSVal x = Invoke(&stack_pointer, oparg, &e);
          sp = stack_pointer;
          PUSH(x);
          if (e) {
            break;
          }
        }
        continue;
      }

      case OP::CONSTRUCT: {
        const int argc = oparg;
        const JSVal v = sp[-(argc + 2)];
        if (!v.IsCallable()) {
          e.Report(Error::Type, "not callable object");
          break;
        }
        JSFunction* func = v.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          frame = stack_.NewCodeFrame(
              ctx_,
              static_cast<JSVMFunction*>(func)->code(),
              static_cast<JSVMFunction*>(func)->scope(), instr, argc);
          if (!frame) {
            e.Report(Error::Range, "maximum call stack size exceeded");
            break;
          }
          code = frame->code();
          first_instr = frame->data();
          instr = first_instr;
          sp = frame->stacktop();
          dynamic_env_level = 0;
          constants = &frame->constants();
          names = &code->names();
          strict = code->strict();
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_, frame, ERR);
        } else {
          // Native Function, so use Invoke
          JSVal* stack_pointer = sp;
          const JSVal x = Construct(&stack_pointer, oparg, &e);
          sp = stack_pointer;
          PUSH(x);
          if (e) {
            break;
          }
        }
        continue;
      }

      case OP::EVAL: {
        // maybe direct call to eval
        const int argc = oparg;
        const JSVal v = sp[-(argc + 2)];
        if (!v.IsCallable()) {
          e.Report(Error::Type, "not callable object");
          break;
        }
        JSFunction* func = v.object()->AsCallable();
        if (!func->IsNativeFunction()) {
          // inline call
          frame = stack_.NewCodeFrame(
              ctx_,
              static_cast<JSVMFunction*>(func)->code(),
              static_cast<JSVMFunction*>(func)->scope(), instr, argc);
          if (!frame) {
            e.Report(Error::Range, "maximum call stack size exceeded");
            break;
          }
          code = frame->code();
          first_instr = frame->data();
          instr = first_instr;
          sp = frame->stacktop();
          dynamic_env_level = 0;
          constants = &frame->constants();
          names = &code->names();
          strict = code->strict();
          static_cast<JSVMFunction*>(func)->InstantiateBindings(ctx_, frame, ERR);
        } else {
          // Native Function, so use Invoke
          JSVal* stack_pointer = sp;
          const JSVal x = InvokeMaybeEval(&stack_pointer, oparg, &e);
          sp = stack_pointer;
          PUSH(x);
          if (e) {
            break;
          }
        }
        continue;
      }

      case OP::CALL_NAME: {
        const Symbol& s = GETITEM(names, oparg);
        JSVal res;
        if (JSEnv* target_env = GetEnv(frame->lexical_env(), s)) {
          const JSVal w = target_env->GetBindingValue(ctx_, s, false, ERR);
          PUSH(w);
          PUSH(target_env->ImplicitThisValue());
        } else {
          RaiseReferenceError(s, &e);
          break;
        }
        continue;
      }

      case OP::CALL_ELEMENT: {
        const JSVal element = POP();
        const JSVal base = TOP();
        const JSVal res = LoadElement(sp, base, element, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        continue;
      }

      case OP::CALL_PROP: {
        const JSVal base = TOP();
        const Symbol& s = GETITEM(names, oparg);
        const JSVal res = LoadProp(sp, base, s, strict, ERR);
        SET_TOP(res);
        PUSH(base);
        continue;
      }
    }  // switch

    // search exception handler or finally handler.
    // if finally handler found, set value to notify that RETURN_SUBROUTINE
    // should rethrow exception.
    assert(e);
    typedef Code::ExceptionTable ExceptionTable;
    const ExceptionTable& table = code->exception_table();
    bool handler_found = false;
    for (ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      const int handler = std::get<0>(*it);
      const uint16_t begin = std::get<1>(*it);
      const uint16_t end = std::get<2>(*it);
      const uint16_t stack_base_level = std::get<3>(*it);
      const uint16_t env_level = std::get<4>(*it);
      const uint32_t offset = static_cast<uint32_t>(instr - first_instr);
      if (begin < offset && offset <= end) {
        const JSVal error = JSError::Detail(ctx_, &e);
        e.Clear();
        UNWIND_STACK(stack_base_level);
        UNWIND_DYNAMIC_ENV(env_level);
        PUSH(error);
        if (handler == Handler::FINALLY) {
          // finally jump if return or error raised
          PUSH(JSFalse);
        }
        JUMPTO(end);
        handler_found = true;
        break;
      }
    }
    if (handler_found) {
      continue;
    }
    break;
  }  // for main loop
  const JSVal error = JSError::Detail(ctx_, &e);
  return std::make_pair(error, THROW);
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
