#ifndef IV_LV5_BREAKER_STUB_H_
#define IV_LV5_BREAKER_STUB_H_
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/jsfunction.h>
#include <iv/lv5/breaker/context_fwd.h>
#include <iv/lv5/breaker/templates.h>
#include <iv/lv5/accessor.h>
namespace iv {
namespace lv5 {
namespace breaker {
namespace stub {

#define IV_LV5_BREAKER_RAISE_PAIR()\
  do {\
    void* pc = *stack->ret;\
    *stack->ret = Templates<>::dispatch_exception_handler();  /* NOLINT */\
    return Extract(core::BitCast<uint64_t>(pc), static_cast<uint64_t>(0));\
  } while (0)

#define IV_LV5_BREAKER_RAISE()\
  do {\
    void* pc = *stack->ret;\
    *stack->ret = Templates<>::dispatch_exception_handler();  /* NOLINT */\
    return core::BitCast<uint64_t>(pc);\
  } while (0)

#define IV_LV5_BREAKER_ERR\
  stack->error);\
  do {\
    if (*stack->error) {\
      IV_LV5_BREAKER_RAISE();\
    }\
  } while (0);\
((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define IV_LV5_BREAKER_ERR_PAIR\
  stack->error);\
  do {\
    if (*stack->error) {\
      IV_LV5_BREAKER_RAISE_PAIR();\
    }\
  } while (0);\
((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

void BUILD_ENV(Context* ctx,
               railgun::Frame* frame,
               uint32_t size, uint32_t mutable_start);

Rep WITH_SETUP(Frame* stack, railgun::Frame* frame, JSVal src);

Rep FORIN_SETUP(Frame* stack, JSVal enumerable);

Rep FORIN_ENUMERATE(Context* ctx, JSVal iterator);

void FORIN_LEAVE(Context* ctx, JSVal iterator);

Rep BINARY_ADD(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_SUBTRACT(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_MULTIPLY(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_DIVIDE(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_MODULO(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_LSHIFT(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_RSHIFT(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_RSHIFT_LOGICAL(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_LT(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_LTE(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_GT(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_GTE(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_INSTANCEOF(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_IN(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_EQ(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_STRICT_EQ(JSVal lhs, JSVal rhs);

Rep BINARY_NE(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_STRICT_NE(JSVal lhs, JSVal rhs);

Rep BINARY_BIT_AND(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_BIT_XOR(Frame* stack, JSVal lhs, JSVal rhs);

Rep BINARY_BIT_OR(Frame* stack, JSVal lhs, JSVal rhs);

Rep TO_OBJECT(Frame* stack, JSVal src);

Rep TO_NUMBER(Frame* stack, JSVal src);

Rep UNARY_NEGATIVE(Frame* stack, JSVal src);

JSVal UNARY_NOT(JSVal src);

Rep UNARY_BIT_NOT(Frame* stack, JSVal src);

Rep THROW(Frame* stack, JSVal src);

Rep THROW_WITH_TYPE_AND_MESSAGE(Frame* stack,
                                Error::Code type, const char* message);

void RaiseReferenceError(Symbol name, Error* e);

JSEnv* GetEnv(Context* ctx, JSEnv* env, Symbol name);

Rep LOAD_GLOBAL(Frame* stack, Symbol name, GlobalIC* ic, Assembler* as);

Rep STORE_GLOBAL(Frame* stack, Symbol name,
                 GlobalIC* ic, Assembler* as, JSVal src);

Rep DELETE_GLOBAL(Frame* stack, Symbol name);

template<bool Strict>
inline Rep TYPEOF_GLOBAL(Frame* stack, Symbol name) {
  Context* ctx = stack->ctx;
  JSEnv* global = ctx->global_env();
  if (!global->HasBinding(ctx, name)) {
    // not found -> unresolvable reference
    return Extract(ctx->global_data()->string_undefined());
  }
  const JSVal res = global->GetBindingValue(ctx, name, Strict, IV_LV5_BREAKER_ERR);
  return Extract(res.TypeOf(ctx));
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_HEAP(Frame* stack, JSEnv* env, uint32_t offset) {
  JSDeclEnv* decl = static_cast<JSDeclEnv*>(env);
  const JSVal w = decl->GetByOffset(offset, Strict, IV_LV5_BREAKER_ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    decl->SetByOffset(offset, std::get<1>(results), Strict, IV_LV5_BREAKER_ERR);
    return Extract(std::get<Returned>(results));
  } else {
    Context* ctx = stack->ctx;
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, IV_LV5_BREAKER_ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    decl->SetByOffset(offset, std::get<1>(results), Strict, IV_LV5_BREAKER_ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

template<int Target, std::size_t Returned, bool Strict>
inline JSVal IncrementName(Context* ctx,
                           JSEnv* env, Symbol name, Error* e) {
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal w =
        current->GetBindingValue(ctx, name, Strict, IV_LV5_ERROR(e));
    if (w.IsInt32() &&
        railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
      std::tuple<JSVal, JSVal> results;
      const int32_t target = w.int32();
      std::get<0>(results) = w;
      std::get<1>(results) = JSVal::Int32(target + Target);
      current->SetMutableBinding(ctx, name, std::get<1>(results), Strict, e);
      return std::get<Returned>(results);
    } else {
      std::tuple<double, double> results;
      std::get<0>(results) = w.ToNumber(ctx, IV_LV5_ERROR(e));
      std::get<1>(results) = std::get<0>(results) + Target;
      current->SetMutableBinding(ctx, name, std::get<1>(results), Strict, e);
      return std::get<Returned>(results);
    }
  }
  RaiseReferenceError(name, e);
  return 0.0;
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_NAME(Frame* stack, JSEnv* env, Symbol name) {
  const JSVal res =
      IncrementName<Target, Returned, Strict>(stack->ctx, env, name, IV_LV5_BREAKER_ERR);
  return Extract(res);
}

RepPair CALL(Frame* stack,
             JSVal callee,
             JSVal* offset,
             uint64_t argc_with_this,
             railgun::Instruction* instr);

RepPair EVAL(Frame* stack,
             JSVal callee,
             JSVal* offset,
             uint64_t argc_with_this,
             railgun::Instruction* instr,
             railgun::Frame* from);

RepPair CONSTRUCT(Frame* stack,
                  JSVal callee,
                  JSVal* offset,
                  uint64_t argc_with_this,
                  railgun::Instruction* instr);

Rep CONCAT(Frame* stack, JSVal* src, uint32_t count);

Rep RAISE(Frame* stack, Error::Code code, JSString* str);

Rep TYPEOF(Context* ctx, JSVal src);

Rep TO_PRIMITIVE_AND_TO_STRING(Frame* stack, JSVal src);

template<int Type>
inline void STORE_OBJECT_INDEXED(
     Context* ctx, JSVal target, JSVal item, uint32_t index) {
  JSObject* obj = target.object();
  Slot slot;
  Error::Dummy dummy;
  switch (Type) {
    case ObjectLiteral::DATA:
      obj->DefineOwnIndexedPropertySlot(
          ctx, index,
          DataDescriptor(
              item,
              ATTR::W | ATTR::E| ATTR::C),
          &slot, false, &dummy);
      break;
    case ObjectLiteral::GET:
      obj->DefineOwnIndexedPropertySlot(
          ctx, index,
          AccessorDescriptor(
              item.object(),
              nullptr,
              ATTR::E| ATTR::C|
              ATTR::UNDEF_SETTER),
          &slot, false, &dummy);
      break;
    case ObjectLiteral::SET:
      obj->DefineOwnIndexedPropertySlot(
          ctx, index,
          AccessorDescriptor(
              nullptr,
              item.object(),
              ATTR::E| ATTR::C|
              ATTR::UNDEF_GETTER),
          &slot, false, &dummy);
      break;
  }
}

void STORE_OBJECT_GET(Context* ctx, JSVal target, JSVal item, uint32_t offset);

void STORE_OBJECT_SET(Context* ctx, JSVal target, JSVal item, uint32_t offset);

void INIT_VECTOR_ARRAY_ELEMENT(JSVal target, const JSVal* reg,
                               uint32_t index, uint32_t size);

void INIT_SPARSE_ARRAY_ELEMENT(
    JSVal target, const JSVal* reg, uint32_t index, uint32_t size);

template<bool CONFIGURABLE>
inline Rep INSTANTIATE_DECLARATION_BINDING(
    Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (!env->HasBinding(ctx, name)) {
    env->CreateMutableBinding(ctx, name, CONFIGURABLE, IV_LV5_BREAKER_ERR);
  } else if (env == ctx->global_env()) {
    JSObject* const go = ctx->global_obj();
    const PropertyDescriptor existing_prop = go->GetProperty(ctx, name);
    if (existing_prop.IsConfigurable()) {
      go->DefineOwnProperty(
          ctx,
          name,
          DataDescriptor(
              JSUndefined,
              ATTR::W | ATTR::E |
              ((CONFIGURABLE) ? ATTR::C : ATTR::NONE)),
          true, IV_LV5_BREAKER_ERR);
    } else {
      if (existing_prop.IsAccessor()) {
        stack->error->Report(Error::Type,
                             "create mutable function binding failed");
        IV_LV5_BREAKER_RAISE();
      }
      const DataDescriptor* const data = existing_prop.AsDataDescriptor();
      if (!data->IsWritable() || !data->IsEnumerable()) {
        stack->error->Report(Error::Type,
                             "create mutable function binding failed");
        IV_LV5_BREAKER_RAISE();
      }
    }
  }
  return 0;
}

template<bool CONFIGURABLE, bool Strict>
inline Rep INSTANTIATE_VARIABLE_BINDING(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (!env->HasBinding(ctx, name)) {
    env->CreateMutableBinding(ctx, name, CONFIGURABLE, IV_LV5_BREAKER_ERR);
    env->SetMutableBinding(ctx, name, JSUndefined, Strict, IV_LV5_BREAKER_ERR);
  }
  return 0;
}

void INITIALIZE_HEAP_IMMUTABLE(JSEnv* env, JSVal src, uint32_t offset);

Rep INCREMENT(Frame* stack, JSVal src);

Rep DECREMENT(Frame* stack, JSVal src);

Rep POSTFIX_INCREMENT(Frame* stack, JSVal val, JSVal* src);

Rep POSTFIX_DECREMENT(Frame* stack, JSVal val, JSVal* src);

bool TO_BOOLEAN(JSVal src);

template<bool Strict>
inline Rep LOAD_ARGUMENTS(Frame* stack, railgun::Frame* frame) {
  if (Strict) {
    JSObject* obj = JSStrictArguments::New(
        stack->ctx,
        static_cast<JSFunction*>(frame->callee().object()),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        IV_LV5_BREAKER_ERR);
    return Extract(obj);
  } else {
    JSObject* obj = JSNormalArguments::New(
        stack->ctx,
        static_cast<JSFunction*>(frame->callee().object()),
        frame->code()->params(),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        static_cast<JSDeclEnv*>(frame->variable_env()),
        IV_LV5_BREAKER_ERR);
    return Extract(obj);
  }
}

Rep PREPARE_DYNAMIC_CALL(Frame* stack, JSEnv* env, Symbol name, JSVal* base);

template<bool Strict>
inline Rep LOAD_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal res = current->GetBindingValue(ctx, name, Strict, IV_LV5_BREAKER_ERR);
    return Extract(res);
  }
  RaiseReferenceError(name, stack->error);
  IV_LV5_BREAKER_RAISE();
}

template<bool Strict>
inline Rep STORE_NAME(Frame* stack, JSEnv* env, Symbol name, JSVal src) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    current->SetMutableBinding(ctx, name, src, Strict, IV_LV5_BREAKER_ERR);
  } else {
    if (Strict) {
      stack->error->Report(Error::Reference,
                           "putting to unresolvable reference "
                           "not allowed in strict reference");
      IV_LV5_BREAKER_RAISE();
    } else {
      ctx->global_obj()->Put(ctx, name, src, Strict, IV_LV5_BREAKER_ERR);
    }
  }
  return 0;
}

template<bool Strict>
inline Rep DELETE_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    return Extract(JSVal::Bool(current->DeleteBinding(ctx, name)));
  }
  return Extract(JSTrue);
}

template<bool Strict>
inline Rep TYPEOF_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal res = current->GetBindingValue(ctx, name, Strict, IV_LV5_BREAKER_ERR);
    return Extract(res.TypeOf(ctx));
  }
  return Extract(ctx->global_data()->string_undefined());
}

Rep LOAD_ELEMENT(Frame* stack, JSVal base, JSVal element);

template<bool Strict>
void StorePropPrimitive(Context* ctx,
                        JSVal base, Symbol name, JSVal stored, Error* e) {
  assert(base.IsPrimitive());
  Slot slot;
  JSObject* const o = base.ToObject(ctx, IV_LV5_ERROR_VOID(e));
  if (!o->CanPut(ctx, name, &slot)) {
    if (Strict) {
      e->Report(Error::Type, "cannot put value to object");
    }
    return;
  }

  if (slot.IsNotFound() || slot.attributes().IsData()) {
    if (Strict) {
      e->Report(Error::Type, "value to symbol in transient object");
    }
    return;
  }

  const Accessor* ac = slot.accessor();
  assert(ac->setter());
  ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
  args[0] = stored;
  static_cast<JSFunction*>(ac->setter())->Call(&args, base, e);
}

template<bool Strict>
inline void StorePropImpl(Context* ctx,
                          JSVal base, Symbol name, JSVal stored, Error* e) {
  if (base.IsPrimitive()) {
    StorePropPrimitive<Strict>(ctx, base, name, stored, e);
  } else {
    base.object()->Put(ctx, name, stored, Strict, e);
  }
}

Rep STORE_ELEMENT_GENERIC(
    Frame* stack, JSVal base, JSVal src, JSVal element, StoreElementIC* ic);

Rep STORE_ELEMENT_INDEXED(
    Frame* stack, JSVal base, JSVal src, int32_t index, StoreElementIC* ic);

Rep STORE_ELEMENT(
    Frame* stack, JSVal base, JSVal src, JSVal element, StoreElementIC* ic);

template<bool Strict>
inline Rep DELETE_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  if (element.IsInt32()) {
    int32_t index = element.int32();
    if (index >= 0) {
      JSObject* const obj = base.ToObject(ctx, IV_LV5_BREAKER_ERR);
      const bool result = obj->DeleteIndexed(ctx, index, Strict, IV_LV5_BREAKER_ERR);
      return Extract(JSVal::Bool(result));
    }
  }
  const Symbol name = element.ToSymbol(ctx, IV_LV5_BREAKER_ERR);
  JSObject* const obj = base.ToObject(ctx, IV_LV5_BREAKER_ERR);
  const bool result = obj->Delete(ctx, name, Strict, IV_LV5_BREAKER_ERR);
  return Extract(JSVal::Bool(result));
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  const Symbol name = element.ToSymbol(ctx, IV_LV5_BREAKER_ERR);
  Slot slot;
  const JSVal w = base.GetSlot(ctx, name, &slot, IV_LV5_BREAKER_ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), IV_LV5_BREAKER_ERR);
    return Extract(std::get<Returned>(results));
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, IV_LV5_BREAKER_ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), IV_LV5_BREAKER_ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

Rep LOAD_REGEXP(Context* ctx, JSRegExp* reg);

Rep LOAD_OBJECT(Context* ctx, Map* map);

Rep LOAD_ARRAY(Context* ctx, uint32_t len);

Rep DUP_ARRAY(Context* ctx, const JSVal constant);

JSEnv* TRY_CATCH_SETUP(Context* ctx, JSEnv* outer, Symbol sym, JSVal value);

Rep STORE_PROP_GENERIC(Frame* stack,
                       JSVal base, JSVal src, StorePropertyIC* ic);

Rep LOAD_PROP_GENERIC(Frame* stack, JSVal base, LoadPropertyIC* ic);

Rep LOAD_PROP(Frame* stack, JSVal base, LoadPropertyIC* ic);

Rep STORE_PROP(Frame* stack, JSVal base, JSVal src, StorePropertyIC* ic);

template<bool Strict>
inline Rep DELETE_PROP(Frame* stack, JSVal base, Symbol name) {
  Context* ctx = stack->ctx;
  JSObject* const obj = base.ToObject(ctx, IV_LV5_BREAKER_ERR);
  const bool res = obj->Delete(ctx, name, Strict, IV_LV5_BREAKER_ERR);
  return Extract(JSVal::Bool(res));
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_PROP(Frame* stack, JSVal base, Symbol name) {
  Context* ctx = stack->ctx;
  Slot slot;
  const JSVal w = base.GetSlot(ctx, name, &slot, IV_LV5_BREAKER_ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), IV_LV5_BREAKER_ERR);
    return Extract(std::get<Returned>(results));
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, IV_LV5_BREAKER_ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), IV_LV5_BREAKER_ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_STUB_H_
