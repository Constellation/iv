#ifndef _IV_LV5_RAILGUN_VM_FWD_H_
#define _IV_LV5_RAILGUN_VM_FWD_H_
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/stack.h"
namespace iv {
namespace lv5 {
namespace railgun {

class VM {
 public:
  enum Status {
    STOP,
    RETURN,
    THROW
  };

  inline std::pair<JSVal, Status> Run(Code* code, Error* e);
  inline std::pair<JSVal, Status> RunGlobal(Code* code, Error* e);
  inline std::pair<JSVal, Status> RunEval(Code* code,
                                          JSEnv* variable_env,
                                          JSEnv* lexical_env,
                                          JSVal this_binding,
                                          Error* e);
  inline std::pair<JSVal, Status> Execute(Frame* frame, Error* e);
  inline std::pair<JSVal, Status> Execute(const Arguments& args, JSVMFunction* func, Error* e);

  JSVal Invoke(JSFunction* func, JSVal* sp, int argc, Error* e) {
    VMArguments args(ctx_, sp - argc - 1, argc);
    return func->Call(&args, sp[-(argc + 1)], e);
  }

  JSVal InvokeMaybeEval(JSFunction* func, JSVal* sp, int argc, Frame* prev, Error* e) {
    VMArguments args(ctx_, sp - argc - 1, argc);
    const JSAPI native = func->NativeFunction();
    if (native && native == &GlobalEval) {
      // direct call to eval point
      args.set_this_binding(sp[-(argc + 1)]);
      return DirectCallToEval(args, prev, e);
    }
    return func->Call(&args, sp[-(argc + 1)], e);
  }

  JSVal Construct(JSFunction* func, JSVal* sp, int argc, Error* e) {
    VMArguments args(ctx_, sp - argc - 1, argc);
    args.set_constructor_call(true);
    return func->Construct(&args, e);
  }

  void RaiseReferenceError(const Symbol& name, Error* e) const {
    StringBuilder builder;
    builder.Append('"');
    builder.Append(symbol::GetSymbolString(name));
    builder.Append("\" not defined");
    e->Report(Error::Reference, builder.BuildUStringPiece());
  }

  void RaiseImmutable(const Symbol& name, Error* e) const {
    StringBuilder builder;
    builder.Append("mutating immutable binding \"");
    builder.Append(symbol::GetSymbolString(name));
    builder.Append("\" not allowed in strict mode");
    e->Report(Error::Type, builder.BuildUStringPiece());
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
    const Symbol s = GetSymbol(element, CHECK);
    return LoadPropImpl(sp, base, s, strict, e);
  }

  JSVal LoadPropImpl(JSVal* sp, const JSVal& base,
                     const Symbol& s, bool strict, Error* e) {
    if (base.IsPrimitive()) {
      // section 8.7.1 special [[Get]]
      if (base.IsString()) {
        // string short circuit
        JSString* str = base.string();
        if (s == symbol::length()) {
          return JSVal::UInt32(static_cast<uint32_t>(str->size()));
        }
        if (symbol::IsArrayIndexSymbol(s)) {
          const uint32_t index = symbol::GetIndexFromSymbol(s);
          if (index < str->size()) {
            return JSString::NewSingle(ctx_, str->GetAt(index));
          }
        }
      }
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
    const Symbol s = GetSymbol(element, CHECK);
    StorePropImpl(base, s, stored, strict, e);
  }

  void StoreProp(const JSVal& base, const Symbol& s,
                 const JSVal& stored, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    StorePropImpl(base, s, stored, strict, e);
  }

  void StorePropImpl(const JSVal& base, Symbol s,
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
        ac->set()->AsCallable()->Call(&a, base, e);
        return;
      } else {
        if (strict) {
          e->Report(Error::Type, "value to symbol in transient object");
          return;
        }
      }
    } else {
      base.object()->Put(ctx_, s, stored, strict, e);
      return;
    }
  }
#undef CHECK

#define CHECK IV_LV5_ERROR_WITH(e, JSEmpty)

  JSVal BinaryAdd(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    // fast case check
    if (lhs.IsNumber() && rhs.IsNumber()) {
      return lhs.number() + rhs.number();
    }
    if (lhs.IsString()) {
      if (rhs.IsString()) {
        return JSString::New(ctx_, lhs.string(), rhs.string());
      } else {
        const JSVal rp = rhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
        JSString* const rs = rp.ToString(ctx_, CHECK);
        return JSString::New(ctx_, lhs.string(), rs);
      }
    }

    const JSVal lprim = lhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
    const JSVal rprim = rhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
    if (lprim.IsString() || rprim.IsString()) {
      JSString* const lstr = lprim.ToString(ctx_, CHECK);
      JSString* const rstr = rprim.ToString(ctx_, CHECK);
      return JSString::New(ctx_, lstr, rstr);
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
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Int32(lhs.int32() << (rhs.int32() & 0x1f));
    }
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left << (rhs.ToInt32(ctx_, e) & 0x1f));
  }

  JSVal BinaryRShift(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Int32(lhs.int32() >> (rhs.int32() & 0x1f));
    }
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left >> (rhs.ToInt32(ctx_, e) & 0x1f));
  }

  JSVal BinaryRShiftLogical(const JSVal& lhs,
                            const JSVal& rhs, Error* e) const {
    uint32_t left_result;
    if (lhs.GetUInt32(&left_result) && rhs.IsInt32()) {
      return JSVal::UInt32(left_result >> (rhs.int32() & 0x1f));
    }
    const uint32_t left = lhs.ToUInt32(ctx_, CHECK);
    return JSVal::UInt32(left >> (rhs.ToInt32(ctx_, e) & 0x1f));
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
    const Symbol s = GetSymbol(lhs, CHECK);
    return JSVal::Bool(rhs.object()->HasProperty(ctx_, s));
  }

  JSVal BinaryEqual(const JSVal& lhs,
                    const JSVal& rhs, Error* e) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Bool(lhs.int32() == rhs.int32());
    }
    const bool res = internal::AbstractEqual(ctx_, lhs, rhs, CHECK);
    return JSVal::Bool(res);
  }

  JSVal BinaryStrictEqual(const JSVal& lhs,
                          const JSVal& rhs) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Bool(lhs.int32() == rhs.int32());
    }
    return JSVal::Bool(internal::StrictEqual(lhs, rhs));
  }

  JSVal BinaryNotEqual(const JSVal& lhs,
                       const JSVal& rhs, Error* e) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Bool(lhs.int32() != rhs.int32());
    }
    const bool res = internal::AbstractEqual(ctx_, lhs, rhs, CHECK);
    return JSVal::Bool(!res);
  }

  JSVal BinaryStrictNotEqual(const JSVal& lhs,
                             const JSVal& rhs) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Bool(lhs.int32() != rhs.int32());
    }
    return JSVal::Bool(!internal::StrictEqual(lhs, rhs));
  }

  JSVal BinaryBitAnd(const JSVal& lhs,
                     const JSVal& rhs, Error* e) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Int32(lhs.int32() & rhs.int32());
    }
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left & rhs.ToInt32(ctx_, e));
  }

  JSVal BinaryBitXor(const JSVal& lhs,
                     const JSVal& rhs, Error* e) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Int32(lhs.int32() ^ rhs.int32());
    }
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left ^ rhs.ToInt32(ctx_, e));
  }

  JSVal BinaryBitOr(const JSVal& lhs,
                    const JSVal& rhs, Error* e) const {
    if (lhs.IsInt32() && rhs.IsInt32()) {
      return JSVal::Int32(lhs.int32() | rhs.int32());
    }
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left | rhs.ToInt32(ctx_, e));
  }

#undef CHECK


#define CHECK IV_LV5_ERROR_WITH(e, 0.0)

  template<int Target, std::size_t Returned>
  double IncrementName(JSEnv* env, const Symbol& s, bool strict, Error* e) {
    if (JSEnv* current = GetEnv(env, s)) {
      const JSVal w = current->GetBindingValue(ctx_, s, strict, CHECK);
      std::tuple<double, double> results;
      std::get<0>(results) = w.ToNumber(ctx_, CHECK);
      std::get<1>(results) = std::get<0>(results) + Target;
      current->SetMutableBinding(ctx_, s,
                                 std::get<1>(results), strict, CHECK);
      return std::get<Returned>(results);
    }
    RaiseReferenceError(s, e);
    return 0.0;
  }

  template<int Target, std::size_t Returned>
  double IncrementElement(JSVal* sp, const JSVal& base,
                          const JSVal& element, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const Symbol s = GetSymbol(element, CHECK);
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx_, CHECK);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl(base, s, std::get<1>(results), strict, CHECK);
    return std::get<Returned>(results);
  }

  template<int Target, std::size_t Returned>
  double IncrementProp(JSVal* sp, const JSVal& base,
                       const Symbol& s, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx_, CHECK);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl(base, s, std::get<1>(results), strict, CHECK);
    return std::get<Returned>(results);
  }

  Stack* stack() {
    return &stack_;
  }

  const Stack* stack() const {
    return &stack_;
  }

  void set_context(Context* ctx) {
    ctx_ = ctx;
    stack_.SetThis(ctx_->global_obj());
  }

  Symbol GetSymbol(const JSVal& val, Error* e) const {
    uint32_t index;
    if (val.GetUInt32(&index)) {
      return symbol::MakeSymbolFromIndex(index);
    } else {
      const JSString* str =
          val.ToString(ctx_, IV_LV5_ERROR_WITH(e, symbol::length()));
      return context::Intern(ctx_, str);
    }
  }

#undef CHECK

 private:
  Context* ctx_;
  Stack stack_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_VM_FWD_H_
