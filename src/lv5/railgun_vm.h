#ifndef _IV_LV5_RAILGUN_VM_H_
#define _IV_LV5_RAILGUN_VM_H_
#include <cstddef>
#include <vector>
#include <string>
#include <tr1/memory>
#include <tr1/cstdint>
#include "lv5.h"
#include "lv5/gc_template.h"
#include "lv5/jsval.h"
#include "lv5/jsarray.h"
#include "lv5/jsregexp.h"
#include "lv5/property.h"
#include "lv5/internal.h"
#include "lv5/stack_resource.h"
#include "lv5/railgun_fwd.h"
#include "lv5/railgun_op.h"
#include "lv5/railgun_code.h"

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

 private:
  Code* code_;
  std::size_t lineno_;
  JSVal* stacktop_;
  JSEnv* env_;
  Frame* back_;
};

class VM {
 public:
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
    Execute(&frame);
    return EXIT_SUCCESS;
  }

  void Execute(Frame* frame) {
    int opcode;
    uint32_t oparg;
    const Code& code = frame->code();
    const uint8_t* const first_instr = frame->data();
    const uint8_t* instr = first_instr;
    JSVal* sp = frame->stacktop();
    JSEnv* env = frame->env();
    const JSVals& constants = frame->constants();
    const Code::Names& names = code.names();
#define INSTR_OFFSET() reinterpret_cast<uint8_t*>(instr - first_instr)
#define NEXTOP() (*instr++)
#define NEXTARG() (instr += 2, (instr[-1] << 8) + instr[-2])
// #define NEXTARGEX() (instr += 4, (instr[-1] << 24) + (instr[-2] << 16) + (instr[-3] << 8) + instr[-4])
#define PEEKARG() ((instr[2] << 8) + instr[1])
// #define PEEKARGEX() ((instr[4] << 24) + (instr[3] << 16) + (instr[2] << 8) + instr[1])
#define JUMPTO(x) (instr = first_instr + (x))
#define JUMPBY(x) (instr += (x))
#define PUSH(x) (*sp++ = (x))
#define POP() (*--sp)
#define POP_UNUSED() (--sp)
#define STACKADJ(n) (sp += (n))
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
      opcode = NEXTOP();
      oparg = (OP::HasArg(opcode)) ?  NEXTARG() : 0;

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
          const JSVal w = LoadName<false>(env, s, &e);
          if (e) {
            break;
          }
          PUSH(w);
          continue;
        }

        case OP::LOAD_NAME_STRICT: {
          const Symbol& s = GETITEM(names, oparg);
          const JSVal w = LoadName<true>(env, s, &e);
          if (e) {
            break;
          }
          PUSH(w);
          continue;
        }

        case OP::LOAD_ELEMENT: {
          const JSVal element = POP();
          const JSVal base = TOP();
          base.CheckObjectCoercible(&e);
          if (e) {
            break;
          }
          const JSString* str = element.ToString(ctx_, &e);
          if (e) {
            break;
          }
          const Symbol& s = context::Intern(ctx_, str->value());
          if (base.IsPrimitive()) {
            const JSVal res = GetElement<false>(sp, base, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          } else {
            const JSVal res = base.object()->Get(ctx_, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          }
        }

        case OP::LOAD_ELEMENT_STRICT: {
          const JSVal element = POP();
          const JSVal base = TOP();
          base.CheckObjectCoercible(&e);
          if (e) {
            break;
          }
          const JSString* str = element.ToString(ctx_, &e);
          if (e) {
            break;
          }
          const Symbol& s = context::Intern(ctx_, str->value());
          if (base.IsPrimitive()) {
            const JSVal res = GetElement<true>(sp, base, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          } else {
            const JSVal res = base.object()->Get(ctx_, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          }
        }

        case OP::LOAD_PROP: {
          const JSVal base = TOP();
          const Symbol& s = GETITEM(names, oparg);
          base.CheckObjectCoercible(&e);
          if (e) {
            break;
          }
          if (base.IsPrimitive()) {
            const JSVal res = GetElement<false>(sp, base, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          } else {
            const JSVal res =
                base.object()->Get(ctx_, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          }
        }

        case OP::LOAD_PROP_STRICT: {
          const JSVal base = TOP();
          const Symbol& s = GETITEM(names, oparg);
          base.CheckObjectCoercible(&e);
          if (e) {
            break;
          }
          if (base.IsPrimitive()) {
            const JSVal res = GetElement<true>(sp, base, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          } else {
            const JSVal res =
                base.object()->Get(ctx_, s, &e);
            if (e) {
              break;
            }
            SET_TOP(res);
            continue;
          }
        }

        case OP::STORE_NAME: {
          const Symbol& s = GETITEM(names, oparg);
          const JSVal v = POP();
          if (JSEnv* current = GetEnv(env, s)) {
            current->SetMutableBinding(ctx_, s, v, false, &e);
          } else {
            ctx_->global_obj()->Put(ctx_, s, v, false, &e);
          }
          if (e) {
            break;
          }
          continue;
        }

        case OP::STORE_NAME_STRICT: {
          const Symbol& s = GETITEM(names, oparg);
          const JSVal v = POP();
          if (JSEnv* current = GetEnv(env, s)) {
            current->SetMutableBinding(ctx_, s, v, true, &e);
          } else {
            e.Report(Error::Reference,
                     "putting to unresolvable reference "
                     "not allowed in strict reference");
          }
          if (e) {
            break;
          }
          continue;
        }

        case OP::POP_TOP: {
          POP_UNUSED();
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
          // TODO(Constellation) implement it
          // PUSH(ctx_->this_binding());
          PUSH(JSUndefined);
          continue;
        }

        case OP::UNARY_POSITIVE: {
          const JSVal v = TOP();
          const double x = v.ToNumber(ctx_, &e);
          SET_TOP(x);
          if (e) {
            break;
          }
          continue;
        }

        case OP::UNARY_NEGATIVE: {
          const JSVal v = TOP();
          const double x = v.ToNumber(ctx_, &e);
          SET_TOP(-x);
          if (e) {
            break;
          }
          continue;
        }

        case OP::UNARY_NOT: {
          const JSVal v = TOP();
          const bool x = v.ToBoolean(&e);
          if (e) {
            break;
          }
          SET_TOP(JSVal::Bool(!x));
          continue;
        }

        case OP::UNARY_BIT_NOT: {
          const JSVal v = TOP();
          const double value = v.ToNumber(ctx_, &e);
          if (e) {
            break;
          }
          SET_TOP(~core::DoubleToInt32(value));
          continue;
        }

        case OP::BINARY_ADD: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryAdd(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_SUBSTRACT: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinarySub(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_MULTIPLY: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryMultiply(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_DIVIDE: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryDivide(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_MODULO: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryModulo(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_LSHIFT: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryLShift(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_RSHIFT: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryRShift(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_RSHIFT_LOGICAL: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryRShiftLogical(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_LT: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryCompareLT(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_LTE: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryCompareLTE(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_GT: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryCompareGT(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_GTE: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryCompareGTE(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_INSTANCEOF: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryInstanceof(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_IN: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryIn(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_EQ: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryEqual(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_STRICT_EQ: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryStrictEqual(v, w);
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_NE: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryNotEqual(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_STRICT_NE: {
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryStrictNotEqual(v, w);
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_BIT_AND: {  // &
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryBitAnd(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_BIT_XOR: {  // ^
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryBitXor(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BINARY_BIT_OR: {  // |
          const JSVal w = POP();
          const JSVal v = TOP();
          const JSVal res = BinaryBitOr(v, w, &e);
          if (e) {
            break;
          }
          SET_TOP(res);
          continue;
        }

        case OP::BUILD_ARRAY: {
          JSArray* x = JSArray::ReservedNew(ctx_, oparg);
          if (x) {
            PUSH(x);
            continue;
          }
          break;
        }

        case OP::INIT_ARRAY_ELEMENT: {
          const JSVal w = POP();
          JSArray* ary = static_cast<JSArray*>(TOP().object());
          ary->Set(oparg, w);
          continue;
        }

        case OP::BUILD_OBJECT: {
          JSObject* x = JSObject::New(ctx_);
          if (x) {
            PUSH(x);
            continue;
          }
          break;
        }

        case OP::BUILD_REGEXP: {
          const JSVal w = POP();
          JSRegExp* x = JSRegExp::New(ctx_, static_cast<JSRegExp*>(w.object()));
          if (x) {
            PUSH(x);
            continue;
          }
          break;
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
              false, &e);
          if (e) {
            break;
          }
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
              false, &e);
          if (e) {
            break;
          }
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
              false, &e);
          if (e) {
            break;
          }
          continue;
        }

        case OP::CALL: {
          JSVal* stack_pointer = sp;
          const JSVal x = CallFunction(&stack_pointer, oparg, &e);
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
            const JSVal w = target_env->GetBindingValue(ctx_, s, false, &e);
            if (e) {
              break;
            }
            PUSH(w);
            PUSH(target_env->ImplicitThisValue());
          } else {
            RaiseReferenceError(s, &e);
            break;
          }
          continue;
        }
      }  // switch

      // error found
      break;
    }  // for main loop
#undef NEXTOP
#undef NEXTARG
#undef PEEKARG
#undef JUMPTO
#undef JUMPBY
#undef PUSH
#undef POP
#undef STACKADJ
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

  JSVal CallFunction(JSVal** stack_pointer, int argc, Error* e) {
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

  template<bool Strict>
  JSVal LoadName(JSEnv* env, const Symbol& name, Error* e) const {
    if (JSEnv* current = GetEnv(env, name)) {
      return current->GetBindingValue(ctx_, name, Strict, e);
    }
    RaiseReferenceError(name, e);
    return JSEmpty;
  }

#define CHECK ERROR_WITH(e, JSEmpty)

  template<bool Strict>
  JSVal GetElement(JSVal* stack_pointer,
                   const JSVal& base,
                   const Symbol& name,
                   Error* e) const {
    // section 8.7.1 special [[Get]]
    const JSObject* const o = base.ToObject(ctx_, CHECK);
    const PropertyDescriptor desc = o->GetProperty(ctx_, name);
    if (desc.IsEmpty()) {
      return JSUndefined;
    }
    if (desc.IsDataDescriptor()) {
      return desc.AsDataDescriptor()->value();
    } else {
      assert(desc.IsAccessorDescriptor());
      const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
      if (ac->get()) {
        VMArguments args(ctx_, stack_pointer - 1, 0);
        const JSVal res = ac->get()->AsCallable()->Call(&args, base, CHECK);
        return res;
      } else {
        return JSUndefined;
      }
    }
  }

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

 private:
  Context* ctx_;
  StackResource stack_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_VM_H_
