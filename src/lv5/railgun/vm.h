#ifndef _IV_LV5_RAILGUN_VM_H_
#define _IV_LV5_RAILGUN_VM_H_
#include <cstddef>
#include <vector>
#include <string>
#include <tr1/memory>
#include <tr1/cstdint>
#include <tr1/tuple>
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
#include "lv5/railgun/op.h"
#include "lv5/railgun/code.h"
#include "lv5/railgun/context.h"
#include "lv5/railgun/jsfunction.h"

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

class Frame {
 public:
  friend class VM;
  const Code& code() const {
    return *code_;
  }

  const uint8_t* data() const {
    return code_->data();
  }

  const JSVals& constants() const {
    return code_->constants();
  }

  JSVal* stacktop() const {
    return stacktop_;
  }

  JSEnv* env() const {
    return env_;
  }

  void set_this_binding(const JSVal& this_binding) {
    this_binding_ = this_binding;
  }

  const JSVal& this_binding() const {
    return this_binding_;
  }

 private:
  Code* code_;
  std::size_t lineno_;
  JSVal* stacktop_;
  JSEnv* env_;
  JSVal this_binding_;
  Frame* back_;
};

class VM {
 public:
  enum Status {
    NORMAL,
    RETURN,
    THROW
  };

  explicit VM(Context* ctx)
    : ctx_(ctx),
      stack_() {
  }

  int Run(Code* code) {
    Frame frame;
    frame.code_ = code;
    frame.stacktop_ = stack_.stack()->Gain(10000);
    frame.env_ = ctx_->variable_env();
    frame.back_ = NULL;
    frame.set_this_binding(JSUndefined);
    Execute(&frame);
    return EXIT_SUCCESS;
  }

  std::pair<JSVal, Status> Execute(Frame* frame) {
    const Code& code = frame->code();
    const uint8_t* const first_instr = frame->data();
    const uint8_t* instr = first_instr;
    JSVal* sp = frame->stacktop();
    JSEnv* env = frame->env();
    uint32_t dynamic_env_level = 0;
    const JSVals& constants = frame->constants();
    const Code::Names& names = code.names();
    const bool strict = code.strict();
    JSVal ret = JSUndefined;
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
#define UNWIND_STACK(n) (sp = (frame->stacktop() + ((n) * 2)))
#define UNWIND_DYNAMIC_ENV(n)\
  do {\
    const uint16_t dynamic_env_level_shrink = (n);\
    assert(dynamic_env_level >= dynamic_env_level_shrink);\
    for (uint16_t i = dynamic_env_level_shrink; i < dynamic_env_level; ++i) {\
      env = env->outer();\
    }\
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

#define GETITEM(target, i) ((target)[(i)])

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
          const JSVal w = LoadName(env, s, strict, ERR);
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
          StoreName(env, s, v, strict, ERR);
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
          if (JSEnv* current = GetEnv(env, s)) {
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
          ret = POP();
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
          PUSH(frame->this_binding());
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
          if (JSEnv* current = GetEnv(env, s)) {
            const JSVal expr = current->GetBindingValue(ctx_, s, strict, ERR);
            PUSH(expr.TypeOf(ctx_));
            continue;
          }
          // unresolvable reference
          PUSH(JSString::NewAsciiString(ctx_, "undefined"));
          continue;
        }

        case OP::DECREMENT_NAME: {
          const Symbol& s = GETITEM(names, oparg);
          const double result = IncrementName<-1, 1>(env, s, strict, ERR);
          PUSH(result);
          continue;
        }

        case OP::POSTFIX_DECREMENT_NAME: {
          const Symbol& s = GETITEM(names, oparg);
          const double result = IncrementName<-1, 0>(env, s, strict, ERR);
          PUSH(result);
          continue;
        }

        case OP::INCREMENT_NAME: {
          const Symbol& s = GETITEM(names, oparg);
          const double result = IncrementName<1, 1>(env, s, strict, ERR);
          PUSH(result);
          continue;
        }

        case OP::POSTFIX_INCREMENT_NAME: {
          const Symbol& s = GETITEM(names, oparg);
          const double result = IncrementName<1, 0>(env, s, strict, ERR);
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
          ret = TOP();
          return std::make_pair(ret, RETURN);
        }

        case OP::SET_RET_VALUE: {
          ret = POP();
          continue;
        }

        case OP::RETURN_RET_VALUE: {
          return std::make_pair(ret, RETURN);
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
          ret = POP();
          e.Report(ret);
          break;
        }

        case OP::WITH_SETUP: {
          const JSVal val = POP();
          JSObject* const obj = val.ToObject(ctx_, ERR);
          JSObjectEnv* const with_env =
              internal::NewObjectEnvironment(ctx_, obj, env);
          with_env->set_provide_this(true);
          env = with_env;
          ++dynamic_env_level;
          continue;
        }

        case OP::POP_ENV: {
          env = env->outer();
          --dynamic_env_level;
          continue;
        }

        case OP::TRY_CATCH_SETUP: {
          const Symbol& s = GETITEM(names, oparg);
          const JSVal error = POP();
          JSEnv* const catch_env =
              internal::NewDeclarativeEnvironment(ctx_, env);
          catch_env->CreateMutableBinding(ctx_, s, false, ERR);
          catch_env->SetMutableBinding(ctx_, s, error, false, ERR);
          env = catch_env;
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
          Code* target = code.codes()[oparg];
          JSFunction* x = JSVMFunction::New(ctx_, target, env);
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
          JSVal* stack_pointer = sp;
          const JSVal x = Invoke(&stack_pointer, oparg, &e);
          sp = stack_pointer;
          PUSH(x);
          if (e) {
            break;
          }
          continue;
        }

        case OP::CONSTRUCT: {
          JSVal* stack_pointer = sp;
          const JSVal x = Construct(&stack_pointer, oparg, &e);
          sp = stack_pointer;
          PUSH(x);
          if (e) {
            break;
          }
          continue;
        }

        case OP::EVAL: {
          // maybe direct call to eval
          JSVal* stack_pointer = sp;
          const JSVal x = InvokeMaybeEval(&stack_pointer, oparg, &e);
          sp = stack_pointer;
          PUSH(x);
          if (e) {
            break;
          }
          continue;
        }

        case OP::CALL_NAME: {
          const Symbol& s = GETITEM(names, oparg);
          JSVal res;
          if (JSEnv* target_env = GetEnv(env, s)) {
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
      const ExceptionTable& table = code.exception_table();
      bool handler_found = false;
      for (ExceptionTable::const_iterator it = table.begin(),
           last = table.end(); it != last; ++it) {
        const int handler = std::tr1::get<0>(*it);
        const uint16_t begin = std::tr1::get<1>(*it);
        const uint16_t end = std::tr1::get<2>(*it);
        const uint16_t stack_base_level = std::tr1::get<3>(*it);
        const uint16_t env_level = std::tr1::get<4>(*it);
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
      // std::printf("stack depth: %d\n", STACK_DEPTH());
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

  JSVal Invoke(JSVal** stack_pointer, int argc, Error* e) {
    JSVal* sp = *stack_pointer;
    VMArguments args(ctx_, sp - argc - 1, argc);
    const JSVal func = sp[-(argc + 2)];
    if (!func.IsCallable()) {
      e->Report(Error::Type, "not callable object");
      return JSEmpty;
    }
    *stack_pointer = sp - (argc + 2);
    return func.object()->AsCallable()->Call(&args, sp[-(argc + 1)], e);
  }

  JSVal InvokeMaybeEval(JSVal** stack_pointer, int argc, Error* e) {
    JSVal* sp = *stack_pointer;
    VMArguments args(ctx_, sp - argc - 1, argc);
    const JSVal func = sp[-(argc + 2)];
    if (!func.IsCallable()) {
      e->Report(Error::Type, "not callable object");
      return JSEmpty;
    }
    JSFunction* const callable = func.object()->AsCallable();
    const JSAPI native = callable->NativeFunction();
    *stack_pointer = sp - (argc + 2);
    if (native && native == &GlobalEval) {
      // direct call to eval point
      args.set_this_binding(sp[-(argc + 1)]);
      return DirectCallToEval(args, e);
    }
    return callable->AsCallable()->Call(&args, sp[-(argc + 1)], e);
  }

  JSVal Construct(JSVal** stack_pointer, int argc, Error* e) {
    JSVal* sp = *stack_pointer;
    VMArguments args(ctx_, sp - argc - 1, argc);
    const JSVal func = sp[-(argc + 2)];
    if (!func.IsCallable()) {
      e->Report(Error::Type, "not callable object");
      return JSEmpty;
    }
    *stack_pointer = sp - (argc + 2);
    return func.object()->AsCallable()->Construct(&args, e);
  }

  void RaiseReferenceError(const Symbol& name, Error* e) const {
    StringBuilder builder;
    builder.Append('"');
    builder.Append(context::GetSymbolString(ctx_, name));
    builder.Append("\" not defined");
    e->Report(Error::Reference, builder.BuildUStringPiece());
  }

  JSEnv* GetEnv(JSEnv* env, const Symbol& name) const {
    JSEnv* current = env;
    while (current) {
      if (current->HasBinding(ctx_, name)) {
        return current;
      } else {
        current = current->outer();
      }
    }
    return NULL;
  }

#define CHECK IV_LV5_ERROR_WITH(e, JSEmpty)

  JSVal LoadName(JSEnv* env, const Symbol& name, bool strict, Error* e) {
    if (JSEnv* current = GetEnv(env, name)) {
      return current->GetBindingValue(ctx_, name, strict, e);
    }
    RaiseReferenceError(name, e);
    return JSEmpty;
  }

  JSVal LoadProp(JSVal* sp, const JSVal& base,
                 const Symbol& s, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    return LoadPropImpl(sp, base, s, strict, e);
  }

  JSVal LoadElement(JSVal* sp, const JSVal& base,
                    const JSVal& element, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const JSString* str = element.ToString(ctx_, CHECK);
    const Symbol s = context::Intern(ctx_, str->value());
    return LoadPropImpl(sp, base, s, strict, e);
  }

  JSVal LoadPropImpl(JSVal* sp, const JSVal& base,
                     const Symbol& s, bool strict, Error* e) {
    if (base.IsPrimitive()) {
      // section 8.7.1 special [[Get]]
      const JSObject* const o = base.ToObject(ctx_, CHECK);
      const PropertyDescriptor desc = o->GetProperty(ctx_, s);
      if (desc.IsEmpty()) {
        return JSUndefined;
      }
      if (desc.IsDataDescriptor()) {
        return desc.AsDataDescriptor()->value();
      } else {
        assert(desc.IsAccessorDescriptor());
        const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
        if (ac->get()) {
          VMArguments args(ctx_, sp - 1, 0);
          const JSVal res = ac->get()->AsCallable()->Call(&args, base, CHECK);
          return res;
        } else {
          return JSUndefined;
        }
      }
    } else {
      return base.object()->Get(ctx_, s, e);
    }
  }

#undef CHECK

#define CHECK IV_LV5_ERROR_VOID(e)

  void StoreName(JSEnv* env, const Symbol& name,
                 const JSVal& stored, bool strict, Error* e) {
    if (JSEnv* current = GetEnv(env, name)) {
      current->SetMutableBinding(ctx_, name, stored, strict, e);
    } else {
      if (strict) {
        e->Report(Error::Reference,
                  "putting to unresolvable reference "
                  "not allowed in strict reference");
      } else {
        ctx_->global_obj()->Put(ctx_, name, stored, strict, e);
      }
    }
  }

  void StoreElement(const JSVal& base, const JSVal& element,
                    const JSVal& stored, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const JSString* str = element.ToString(ctx_, CHECK);
    const Symbol s = context::Intern(ctx_, str->value());
    StorePropImpl(base, s, stored, strict, e);
  }

  void StoreProp(const JSVal& base, const Symbol& s,
                 const JSVal& stored, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    StorePropImpl(base, s, stored, strict, e);
  }

  void StorePropImpl(const JSVal& base, const Symbol& s,
                     const JSVal& stored, bool strict, Error* e) {
    if (base.IsPrimitive()) {
      JSObject* const o = base.ToObject(ctx_, CHECK);
      if (!o->CanPut(ctx_, s)) {
        if (strict) {
          e->Report(Error::Type, "cannot put value to object");
          return;
        }
        return;
      }
      const PropertyDescriptor own_desc = o->GetOwnProperty(ctx_, s);
      if (!own_desc.IsEmpty() && own_desc.IsDataDescriptor()) {
        if (strict) {
          e->Report(Error::Type,
                    "value to symbol defined and not data descriptor");
          return;
        }
        return;
      }
      const PropertyDescriptor desc = o->GetProperty(ctx_, s);
      if (!desc.IsEmpty() && desc.IsAccessorDescriptor()) {
        ScopedArguments a(ctx_, 1, CHECK);
        a[0] = stored;
        const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
        assert(ac->set());
        ac->set()->AsCallable()->Call(&a, base, CHECK);
      } else {
        if (strict) {
          e->Report(Error::Type, "value to symbol in transient object");
          return;
        }
      }
    } else {
      base.object()->Put(ctx_, s, stored, strict, CHECK);
    }
  }
#undef CHECK

#define CHECK IV_LV5_ERROR_WITH(e, JSEmpty)

  JSVal BinaryAdd(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const JSVal lprim = lhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
    const JSVal rprim = rhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
    if (lprim.IsString() || rprim.IsString()) {
      StringBuilder builder;
      const JSString* const lstr = lprim.ToString(ctx_, CHECK);
      const JSString* const rstr = rprim.ToString(ctx_, CHECK);
      builder.Append(*lstr);
      builder.Append(*rstr);
      return builder.Build(ctx_);
    }
    const double left_num = lprim.ToNumber(ctx_, CHECK);
    const double right_num = rprim.ToNumber(ctx_, CHECK);
    return left_num + right_num;
  }

  JSVal BinarySub(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return left_num - right_num;
  }

  JSVal BinaryMultiply(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return left_num * right_num;
  }

  JSVal BinaryDivide(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return left_num / right_num;
  }

  JSVal BinaryModulo(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return std::fmod(left_num, right_num);
  }

  JSVal BinaryLShift(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return core::DoubleToInt32(left_num)
        << (core::DoubleToInt32(right_num) & 0x1f);
  }

  JSVal BinaryRShift(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return core::DoubleToInt32(left_num)
        >> (core::DoubleToInt32(right_num) & 0x1f);
  }

  JSVal BinaryRShiftLogical(const JSVal& lhs,
                            const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    const uint32_t res = core::DoubleToUInt32(left_num)
        >> (core::DoubleToInt32(right_num) & 0x1f);
    return static_cast<double>(res);
  }

  JSVal BinaryCompareLT(const JSVal& lhs,
                        const JSVal& rhs, Error* e) const {
    const internal::CompareKind res =
        internal::Compare<true>(ctx_, lhs, rhs, CHECK);
    return JSVal::Bool(res == internal::CMP_TRUE);
  }

  JSVal BinaryCompareLTE(const JSVal& lhs,
                         const JSVal& rhs, Error* e) const {
    const internal::CompareKind res =
        internal::Compare<false>(ctx_, rhs, lhs, CHECK);
    return JSVal::Bool(res == internal::CMP_FALSE);
  }

  JSVal BinaryCompareGT(const JSVal& lhs,
                        const JSVal& rhs, Error* e) const {
    const internal::CompareKind res =
        internal::Compare<false>(ctx_, rhs, lhs, CHECK);
    return JSVal::Bool(res == internal::CMP_TRUE);
  }

  JSVal BinaryCompareGTE(const JSVal& lhs,
                         const JSVal& rhs, Error* e) const {
    const internal::CompareKind res =
        internal::Compare<true>(ctx_, lhs, rhs, CHECK);
    return JSVal::Bool(res == internal::CMP_FALSE);
  }

  JSVal BinaryInstanceof(const JSVal& lhs,
                         const JSVal& rhs, Error* e) const {
    if (!rhs.IsObject()) {
      e->Report(Error::Type, "instanceof requires object");
      return JSEmpty;
    }
    JSObject* const robj = rhs.object();
    if (!robj->IsCallable()) {
      e->Report(Error::Type, "instanceof requires constructor");
      return JSEmpty;
    }
    const bool res = robj->AsCallable()->HasInstance(ctx_, lhs, CHECK);
    return JSVal::Bool(res);
  }

  JSVal BinaryIn(const JSVal& lhs,
                 const JSVal& rhs, Error* e) const {
    if (!rhs.IsObject()) {
      e->Report(Error::Type, "in requires object");
      return JSEmpty;
    }
    const JSString* const name = lhs.ToString(ctx_, CHECK);
    const bool res =
        rhs.object()->HasProperty(ctx_,
                                  context::Intern(ctx_, name->value()));
    return JSVal::Bool(res);
  }

  JSVal BinaryEqual(const JSVal& lhs,
                    const JSVal& rhs, Error* e) const {
    const bool res = internal::AbstractEqual(ctx_, lhs, rhs, CHECK);
    return JSVal::Bool(res);
  }

  JSVal BinaryStrictEqual(const JSVal& lhs,
                          const JSVal& rhs) const {
    return JSVal::Bool(internal::StrictEqual(lhs, rhs));
  }

  JSVal BinaryNotEqual(const JSVal& lhs,
                       const JSVal& rhs, Error* e) const {
    const bool res = internal::AbstractEqual(ctx_, lhs, rhs, CHECK);
    return JSVal::Bool(!res);
  }

  JSVal BinaryStrictNotEqual(const JSVal& lhs,
                             const JSVal& rhs) const {
    return JSVal::Bool(!internal::StrictEqual(lhs, rhs));
  }

  JSVal BinaryBitAnd(const JSVal& lhs,
                     const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return core::DoubleToInt32(left_num) & (core::DoubleToInt32(right_num));
  }

  JSVal BinaryBitXor(const JSVal& lhs,
                     const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return core::DoubleToInt32(left_num) ^ (core::DoubleToInt32(right_num));
  }

  JSVal BinaryBitOr(const JSVal& lhs,
                    const JSVal& rhs, Error* e) const {
    const double left_num = lhs.ToNumber(ctx_, CHECK);
    const double right_num = rhs.ToNumber(ctx_, CHECK);
    return core::DoubleToInt32(left_num) | (core::DoubleToInt32(right_num));
  }

#undef CHECK


#define CHECK IV_LV5_ERROR_WITH(e, 0.0)

  template<int Target, std::size_t Returned>
  double IncrementName(JSEnv* env, const Symbol& s, bool strict, Error* e) {
    if (JSEnv* current = GetEnv(env, s)) {
      const JSVal w = current->GetBindingValue(ctx_, s, strict, CHECK);
      std::tr1::tuple<double, double> results;
      std::tr1::get<0>(results) = w.ToNumber(ctx_, CHECK);
      std::tr1::get<1>(results) = std::tr1::get<0>(results) + Target;
      current->SetMutableBinding(ctx_, s,
                                 std::tr1::get<1>(results), strict, CHECK);
      return std::tr1::get<Returned>(results);
    }
    RaiseReferenceError(s, e);
    return 0.0;
  }

  template<int Target, std::size_t Returned>
  double IncrementElement(JSVal* sp, const JSVal& base,
                          const JSVal& element, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const JSString* str = element.ToString(ctx_, CHECK);
    const Symbol s = context::Intern(ctx_, str->value());
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    std::tr1::tuple<double, double> results;
    std::tr1::get<0>(results) = w.ToNumber(ctx_, CHECK);
    std::tr1::get<1>(results) = std::tr1::get<0>(results) + Target;
    StorePropImpl(base, s, std::tr1::get<1>(results), strict, CHECK);
    return std::tr1::get<Returned>(results);
  }

  template<int Target, std::size_t Returned>
  double IncrementProp(JSVal* sp, const JSVal& base,
                       const Symbol& s, bool strict, Error* e) {
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    std::tr1::tuple<double, double> results;
    std::tr1::get<0>(results) = w.ToNumber(ctx_, CHECK);
    std::tr1::get<1>(results) = std::tr1::get<0>(results) + Target;
    StorePropImpl(base, s, std::tr1::get<1>(results), strict, CHECK);
    return std::tr1::get<Returned>(results);
  }

#undef CHECK

 private:
  Context* ctx_;
  StackResource stack_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_VM_H_
