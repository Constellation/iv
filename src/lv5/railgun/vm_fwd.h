#ifndef _IV_LV5_RAILGUN_VM_FWD_H_
#define _IV_LV5_RAILGUN_VM_FWD_H_
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/frame.h"
namespace iv {
namespace lv5 {
namespace railgun {

class VM {
 public:
  enum Status {
    NORMAL,
    RETURN,
    THROW
  };

  inline int Run(Context* ctx, Code* code);
  inline std::pair<JSVal, Status> Execute(OldFrame* frame);

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
    const JSString* str = element.ToString(ctx_, CHECK);
    const Symbol s = context::Intern(ctx_, str->value());
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
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx_, CHECK);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl(base, s, std::get<1>(results), strict, CHECK);
    return std::get<Returned>(results);
  }

#undef CHECK

 private:
  Context* ctx_;
  StackResource stack_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_VM_FWD_H_
