#ifndef IV_LV5_RAILGUN_OPERATION_H_
#define IV_LV5_RAILGUN_OPERATION_H_
#include "arith.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

template<int Target>
inline bool IsIncrementOverflowSafe(int32_t val);

template<>
inline bool IsIncrementOverflowSafe<-1>(int32_t val) {
  return val > INT32_MIN;
}

template<>
inline bool IsIncrementOverflowSafe<1>(int32_t val) {
  return val < INT32_MAX;
}

}  // namespace detail

class Operation {
 public:
  explicit Operation(Context* ctx)
    : ctx_(ctx) {
  }

  JSVal Invoke(JSFunction* func, JSVal* sp, int argc, Error* e) {
    VMArguments args(ctx_, sp - argc - 1, argc);
    return func->Call(&args, sp[-(argc + 1)], e);
  }

  JSVal InvokeMaybeEval(JSFunction* func,
                        JSVal* sp, int argc, Frame* prev, Error* e) {
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

  JSDeclEnv* GetHeapEnv(JSEnv* env, uint32_t scope_nest_count) const {
    while (env) {
      assert(env->AsJSDeclEnv() || env->AsJSStaticEnv());
      if (JSDeclEnv* decl = env->AsJSDeclEnv()) {
        if (decl->scope_nest_count() == scope_nest_count) {
          return decl;
        }
      }
      env = env->outer();
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

  JSVal LoadHeap(JSEnv* env, const Symbol& name,
                 bool strict, uint32_t scope_nest_count,
                 uint32_t offset, Error* e) {
    if (JSDeclEnv* target = GetHeapEnv(env, scope_nest_count)) {
        return target->GetByOffset(offset, strict, e);
    }
    UNREACHABLE();
    return JSEmpty;
  }

  JSVal LoadProp(JSVal* sp, const JSVal& base,
                 const Symbol& s, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    return LoadPropImpl(sp, base, s, strict, e);
  }

  JSVal LoadProp(JSVal* sp, Instruction* instr,
                 OP::Type generic,
                 const JSVal& base,
                 const Symbol& s, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    if (base.IsPrimitive()) {
      return LoadPropPrimitive(sp, base, s, strict, e);
    } else {
      // cache patten
      JSObject* obj = base.object();
      if (instr[2].map == obj->map()) {
        // map is cached, so use previous index code
        return obj->GetBySlotOffset(ctx_, instr[3].value, e);
      } else {
        Slot slot;
        if (obj->GetOwnPropertySlot(ctx_, s, &slot)) {
          if (slot.IsCachable()) {
            instr[2].map = obj->map();
            instr[3].value = slot.offset();
          } else {
            // dispatch generic path
            instr[0] = Instruction::GetOPInstruction(generic);
          }
          return slot.Get(ctx_, obj, CHECK);
        } else {
          instr[2].map = NULL;
          return obj->Get(ctx_, s, e);
        }
      }
    }
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
      return LoadPropPrimitive(sp, base, s, strict, e);
    } else {
      return base.object()->Get(ctx_, s, e);
    }
  }

  JSVal LoadPropPrimitive(JSVal* sp, const JSVal& base,
                          const Symbol& s, bool strict, Error* e) {
    // section 8.7.1 special [[Get]]
    assert(base.IsPrimitive());
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
    JSObject* const o = base.ToObject(ctx_, CHECK);
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
        ScopedArguments args(ctx_, 0, CHECK);
        const JSVal res = ac->get()->AsCallable()->Call(&args, base, CHECK);
        return res;
      } else {
        return JSUndefined;
      }
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

  void StoreHeap(JSEnv* env, const Symbol& name,
                 const JSVal& stored, bool strict,
                 uint32_t scope_nest_count, uint32_t offset, Error* e) {
    if (JSDeclEnv* target = GetHeapEnv(env, scope_nest_count)) {
      target->SetByOffset(offset, stored, strict, e);
      return;
    }
    UNREACHABLE();
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

  void StoreProp(const JSVal& base,
                 Instruction* instr,
                 OP::Type generic,
                 const Symbol& s,
                 const JSVal& stored, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    if (base.IsPrimitive()) {
      StorePropPrimitive(base, s, stored, strict, e);
    } else {
      // cache patten
      JSObject* obj = base.object();
      if (instr[2].map == obj->map()) {
        // map is cached, so use previous index code
        return obj->PutToSlotOffset(ctx_, instr[3].value, stored, strict, e);
      } else {
        Slot slot;
        if (obj->GetOwnPropertySlot(ctx_, s, &slot)) {
          if (slot.IsCachable()) {
            instr[2].map = obj->map();
            instr[3].value = slot.offset();
            obj->PutToSlotOffset(ctx_, instr[3].value, stored, strict, e);
          } else {
            // dispatch generic path
            obj->Put(ctx_, s, stored, strict, e);
            instr[0] = Instruction::GetOPInstruction(generic);
          }
          return;
        } else {
          instr[2].map = NULL;
          obj->Put(ctx_, s, stored, strict, e);
          return;
        }
      }
    }
  }

  void StorePropImpl(const JSVal& base, Symbol s,
                     const JSVal& stored, bool strict, Error* e) {
    if (base.IsPrimitive()) {
      StorePropPrimitive(base, s, stored, strict, e);
    } else {
      base.object()->Put(ctx_, s, stored, strict, e);
      return;
    }
  }

  void StorePropPrimitive(const JSVal& base,
                          const Symbol& s,
                          const JSVal& stored, bool strict, Error* e) {
    assert(base.IsPrimitive());
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
  }
#undef CHECK

#define CHECK IV_LV5_ERROR_WITH(e, JSEmpty)

  JSVal BinaryAdd(const JSVal& lhs, const JSVal& rhs, Error* e) const {
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

    const double left = lprim.ToNumber(ctx_, CHECK);
    return left + rprim.ToNumber(ctx_, e);
  }

  JSVal BinarySub(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left = lhs.ToNumber(ctx_, CHECK);
    return left -  rhs.ToNumber(ctx_, e);
  }

  JSVal BinaryMultiply(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left = lhs.ToNumber(ctx_, CHECK);
    return left * rhs.ToNumber(ctx_, e);
  }

  JSVal BinaryDivide(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left = lhs.ToNumber(ctx_, CHECK);
    return left / rhs.ToNumber(ctx_, e);
  }

  JSVal BinaryModulo(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const double left = lhs.ToNumber(ctx_, CHECK);
    return std::fmod(left, rhs.ToNumber(ctx_, e));
  }

  JSVal BinaryLShift(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left << (rhs.ToInt32(ctx_, e) & 0x1f));
  }

  JSVal BinaryRShift(const JSVal& lhs, const JSVal& rhs, Error* e) const {
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left >> (rhs.ToInt32(ctx_, e) & 0x1f));
  }

  JSVal BinaryRShiftLogical(const JSVal& lhs,
                            const JSVal& rhs, Error* e) const {
    const uint32_t left = lhs.ToUInt32(ctx_, CHECK);
    return JSVal::UInt32(left >> (rhs.ToInt32(ctx_, e) & 0x1f));
  }

  JSVal BinaryCompareLT(const JSVal& lhs,
                        const JSVal& rhs, Error* e) const {
    return JSVal::Bool(
        internal::Compare<true>(ctx_, lhs, rhs, e) == internal::CMP_TRUE);
  }

  JSVal BinaryCompareLTE(const JSVal& lhs,
                         const JSVal& rhs, Error* e) const {
    return JSVal::Bool(
        internal::Compare<false>(ctx_, rhs, lhs, e) == internal::CMP_FALSE);
  }

  JSVal BinaryCompareGT(const JSVal& lhs,
                        const JSVal& rhs, Error* e) const {
    return JSVal::Bool(
        internal::Compare<false>(ctx_, rhs, lhs, e) == internal::CMP_TRUE);
  }

  JSVal BinaryCompareGTE(const JSVal& lhs,
                         const JSVal& rhs, Error* e) const {
    return JSVal::Bool(
        internal::Compare<true>(ctx_, lhs, rhs, e) == internal::CMP_FALSE);
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
    return JSVal::Bool(robj->AsCallable()->HasInstance(ctx_, lhs, e));
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
    return JSVal::Bool(internal::AbstractEqual(ctx_, lhs, rhs, e));
  }

  JSVal BinaryStrictEqual(const JSVal& lhs,
                          const JSVal& rhs) const {
    return JSVal::Bool(internal::StrictEqual(lhs, rhs));
  }

  JSVal BinaryNotEqual(const JSVal& lhs,
                       const JSVal& rhs, Error* e) const {
    return JSVal::Bool(!internal::AbstractEqual(ctx_, lhs, rhs, e));
  }

  JSVal BinaryStrictNotEqual(const JSVal& lhs,
                             const JSVal& rhs) const {
    return JSVal::Bool(!internal::StrictEqual(lhs, rhs));
  }

  JSVal BinaryBitAnd(const JSVal& lhs,
                     const JSVal& rhs, Error* e) const {
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left & rhs.ToInt32(ctx_, e));
  }

  JSVal BinaryBitXor(const JSVal& lhs,
                     const JSVal& rhs, Error* e) const {
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left ^ rhs.ToInt32(ctx_, e));
  }

  JSVal BinaryBitOr(const JSVal& lhs,
                    const JSVal& rhs, Error* e) const {
    const int32_t left = lhs.ToInt32(ctx_, CHECK);
    return JSVal::Int32(left | rhs.ToInt32(ctx_, e));
  }

  template<int Target, std::size_t Returned>
  JSVal IncrementName(JSEnv* env, const Symbol& s, bool strict, Error* e) {
    if (JSEnv* current = GetEnv(env, s)) {
      const JSVal w = current->GetBindingValue(ctx_, s, strict, CHECK);
      if (w.IsInt32() && detail::IsIncrementOverflowSafe<Target>(w.int32())) {
        std::tuple<JSVal, JSVal> results;
        const int32_t target = w.int32();
        std::get<0>(results) = w;
        std::get<1>(results) = JSVal::Int32(target + Target);
        current->SetMutableBinding(
            ctx_, s,
            std::get<1>(results), strict, e);
        return std::get<Returned>(results);
      } else {
        std::tuple<double, double> results;
        std::get<0>(results) = w.ToNumber(ctx_, CHECK);
        std::get<1>(results) = std::get<0>(results) + Target;
        current->SetMutableBinding(ctx_, s,
                                   std::get<1>(results), strict, e);
        return std::get<Returned>(results);
      }
    }
    RaiseReferenceError(s, e);
    return 0.0;
  }

  template<int Target, std::size_t Returned>
  JSVal IncrementHeap(JSEnv* env, const Symbol& s,
                      bool strict, uint32_t scope_nest_count,
                      uint32_t offset, Error* e) {
    if (JSDeclEnv* decl = GetHeapEnv(env, scope_nest_count)) {
      const JSVal w = decl->GetByOffset(offset, strict, CHECK);
      if (w.IsInt32() && detail::IsIncrementOverflowSafe<Target>(w.int32())) {
        std::tuple<JSVal, JSVal> results;
        const int32_t target = w.int32();
        std::get<0>(results) = w;
        std::get<1>(results) = JSVal::Int32(target + Target);
        decl->SetByOffset(offset, std::get<1>(results), strict, e);
        return std::get<Returned>(results);
      } else {
        std::tuple<double, double> results;
        std::get<0>(results) = w.ToNumber(ctx_, CHECK);
        std::get<1>(results) = std::get<0>(results) + Target;
        decl->SetByOffset(offset, std::get<1>(results), strict, e);
        return std::get<Returned>(results);
      }
    }
    UNREACHABLE();
    return JSEmpty;
  }

  template<int Target, std::size_t Returned>
  JSVal IncrementGlobal(JSGlobal* global,
                        std::size_t slot, bool strict, Error* e) {
    const JSVal w = global->GetBySlotOffset(ctx_, slot, e);
    if (w.IsInt32() && detail::IsIncrementOverflowSafe<Target>(w.int32())) {
      std::tuple<JSVal, JSVal> results;
      const int32_t target = w.int32();
      std::get<0>(results) = w;
      std::get<1>(results) = JSVal::Int32(target + Target);
      global->PutToSlotOffset(
          ctx_, slot,
          std::get<1>(results), strict, e);
      return std::get<Returned>(results);
    } else {
      std::tuple<double, double> results;
      std::get<0>(results) = w.ToNumber(ctx_, CHECK);
      std::get<1>(results) = std::get<0>(results) + Target;
      global->PutToSlotOffset(ctx_, slot,
                              std::get<1>(results), strict, e);
      return std::get<Returned>(results);
    }
  }

  template<int Target, std::size_t Returned>
  JSVal IncrementElement(JSVal* sp, const JSVal& base,
                         const JSVal& element, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const Symbol s = GetSymbol(element, CHECK);
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    if (w.IsInt32() && detail::IsIncrementOverflowSafe<Target>(w.int32())) {
      std::tuple<JSVal, JSVal> results;
      const int32_t target = w.int32();
      std::get<0>(results) = w;
      std::get<1>(results) = JSVal::Int32(target + Target);
      StorePropImpl(base, s, std::get<1>(results), strict, e);
      return std::get<Returned>(results);
    } else {
      std::tuple<double, double> results;
      std::get<0>(results) = w.ToNumber(ctx_, CHECK);
      std::get<1>(results) = std::get<0>(results) + Target;
      StorePropImpl(base, s, std::get<1>(results), strict, e);
      return std::get<Returned>(results);
    }
  }

  template<int Target, std::size_t Returned>
  JSVal IncrementProp(JSVal* sp, const JSVal& base,
                      const Symbol& s, bool strict, Error* e) {
    base.CheckObjectCoercible(CHECK);
    const JSVal w = LoadPropImpl(sp, base, s, strict, CHECK);
    if (w.IsInt32() && detail::IsIncrementOverflowSafe<Target>(w.int32())) {
      std::tuple<JSVal, JSVal> results;
      const int32_t target = w.int32();
      std::get<0>(results) = w;
      std::get<1>(results) = JSVal::Int32(target + Target);
      StorePropImpl(base, s, std::get<1>(results), strict, e);
      return std::get<Returned>(results);
    } else {
      std::tuple<double, double> results;
      std::get<0>(results) = w.ToNumber(ctx_, CHECK);
      std::get<1>(results) = std::get<0>(results) + Target;
      StorePropImpl(base, s, std::get<1>(results), strict, e);
      return std::get<Returned>(results);
    }
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

  template<int Target, std::size_t Returned>
  JSVal IncrementGlobal(JSGlobal* global,
                        Instruction* instr,
                        const Symbol& s, bool strict, Error* e) {
    if (instr[2].map == global->map()) {
      // map is cached, so use previous index code
      return IncrementGlobal<Target, Returned>(global, instr[3].value, strict, e);
    } else {
      Slot slot;
      if (global->GetOwnPropertySlot(ctx_, s, &slot)) {
        instr[2].map = global->map();
        instr[3].value = slot.offset();
        return IncrementGlobal<Target, Returned>(global, instr[3].value, strict, e);
      } else {
        instr[2].map = NULL;
        return IncrementName<Target, Returned>(ctx_->global_env(), s, strict, e);
      }
    }
  }

  JSVal LoadGlobal(JSGlobal* global, Instruction* instr, const Symbol& s, bool strict, Error* e) {
    if (instr[2].map == global->map()) {
      // map is cached, so use previous index code
      return global->GetBySlotOffset(ctx_, instr[3].value, e);
    } else {
      Slot slot;
      if (global->GetOwnPropertySlot(ctx_, s, &slot)) {
        if (slot.IsCachable()) {
          instr[2].map = global->map();
          instr[3].value = slot.offset();
        } else {
          // not implemented yet
          UNREACHABLE();
        }
        return slot.Get(ctx_, global, e);
      } else {
        instr[2].map = NULL;
        return LoadName(ctx_->global_env(), s, strict, e);
      }
    }
  }

#undef CHECK

 private:
  Context* ctx_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_OPERATION_H_
