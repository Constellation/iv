// Stub functions implementations
#ifndef IV_LV5_BREAKER_STUB_H_
#define IV_LV5_BREAKER_STUB_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/templates.h>
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/jsfunction.h>
#include <iv/lv5/breaker/context_fwd.h>
#include <iv/lv5/breaker/mono_ic.h>
#include <iv/lv5/breaker/poly_ic.h>
#include <iv/lv5/accessor.h>
namespace iv {
namespace lv5 {
namespace breaker {

inline Rep Extract(JSVal val) {
  return val.Layout().bytes_;
}

namespace stub {

#define IV_LV5_BREAKER_RAISE()\
  do {\
    void* pc = *stack->ret;\
    *stack->ret = Templates<>::dispatch_exception_handler();  /* NOLINT */\
    return core::BitCast<uint64_t>(pc);\
  } while (0)

#define ERR\
  stack->error);\
  do {\
    if (*stack->error) {\
      IV_LV5_BREAKER_RAISE();\
    }\
  } while (0);\
((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

inline void BUILD_ENV(Context* ctx,
                      railgun::Frame* frame,
                      uint32_t size, uint32_t mutable_start) {
  frame->variable_env_ = frame->lexical_env_ =
      JSDeclEnv::New(ctx,
                     frame->lexical_env(),
                     size,
                     frame->code()->names().begin(),
                     mutable_start);
}

inline Rep WITH_SETUP(Frame* stack, railgun::Frame* frame, JSVal src) {
  JSObject* const obj = src.ToObject(stack->ctx, ERR);
  JSObjectEnv* const with_env =
      JSObjectEnv::New(stack->ctx, frame->lexical_env(), obj);
  with_env->set_provide_this(true);
  frame->set_lexical_env(with_env);
  return 0;
}

inline Rep FORIN_SETUP(Frame* stack, JSVal enumerable) {
  Context* ctx = stack->ctx;
  railgun::NativeIterator* it;
  if (enumerable.IsString()) {
    it = stack->ctx->GainNativeIterator(enumerable.string());
  } else {
    JSObject* const obj = enumerable.ToObject(ctx, ERR);
    it = stack->ctx->GainNativeIterator(obj);
  }
  return Extract(JSVal::Cell(it));
}

inline Rep FORIN_ENUMERATE(Context* ctx, JSVal iterator) {
  railgun::NativeIterator* it =
      static_cast<railgun::NativeIterator*>(iterator.cell());
  if (it->Has()) {
    const Symbol sym = it->Get();
    it->Next();
    return Extract(JSString::New(ctx, sym));
  }
  return 0;
}

inline void FORIN_LEAVE(Context* ctx, JSVal iterator) {
  railgun::NativeIterator* it =
      static_cast<railgun::NativeIterator*>(iterator.cell());
  ctx->ReleaseNativeIterator(it);
}

inline Rep BINARY_ADD(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  if (lhs.IsString()) {
    if (rhs.IsString()) {
      JSString* const result = JSString::New(ctx, lhs.string(), rhs.string(), ERR);
      return Extract(result);
    } else {
      const JSVal rp = rhs.ToPrimitive(ctx, Hint::NONE, ERR);
      JSString* const rs = rp.ToString(ctx, ERR);
      JSString* const result = JSString::New(ctx, lhs.string(), rs, ERR);
      return Extract(result);
    }
  }

  const JSVal lprim = lhs.ToPrimitive(ctx, Hint::NONE, ERR);
  const JSVal rprim = rhs.ToPrimitive(ctx, Hint::NONE, ERR);
  if (lprim.IsString() || rprim.IsString()) {
    JSString* const lstr = lprim.ToString(ctx, ERR);
    JSString* const rstr = rprim.ToString(ctx, ERR);
    JSString* const result = JSString::New(ctx, lstr, rstr, ERR);
    return Extract(result);
  }

  const double left = lprim.ToNumber(ctx, ERR);
  const double right = rprim.ToNumber(ctx, ERR);
  return Extract(left + right);
}

inline Rep BINARY_SUBTRACT(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const double left = lhs.ToNumber(ctx, ERR);
  const double res = left -  rhs.ToNumber(ctx, ERR);
  return Extract(res);
}

inline Rep BINARY_MULTIPLY(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const double left = lhs.ToNumber(ctx, ERR);
  const double res = left *  rhs.ToNumber(ctx, ERR);
  return Extract(res);
}

inline Rep BINARY_DIVIDE(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const double left = lhs.ToNumber(ctx, ERR);
  const double res = left / rhs.ToNumber(ctx, ERR);
  return Extract(res);
}

inline Rep BINARY_MODULO(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const double left = lhs.ToNumber(ctx, ERR);
  const double right = rhs.ToNumber(ctx, ERR);
  return Extract(core::math::Modulo(left, right));
}

inline Rep BINARY_LSHIFT(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left << (right & 0x1f)));
}

inline Rep BINARY_RSHIFT(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left >> (right & 0x1f)));
}

inline Rep BINARY_RSHIFT_LOGICAL(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const uint32_t left = lhs.ToUInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::UInt32(left >> (right & 0x1f)));
}

inline Rep BINARY_LT(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const CompareResult res = JSVal::Compare<true>(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(res == CMP_TRUE));
}

inline Rep BINARY_LTE(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const CompareResult res = JSVal::Compare<false>(ctx, rhs, lhs, ERR);
  return Extract(JSVal::Bool(res == CMP_FALSE));
}

inline Rep BINARY_GT(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const CompareResult res = JSVal::Compare<false>(ctx, rhs, lhs, ERR);
  return Extract(JSVal::Bool(res == CMP_TRUE));
}

inline Rep BINARY_GTE(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const CompareResult res = JSVal::Compare<true>(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(res == CMP_FALSE));
}

inline Rep BINARY_INSTANCEOF(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  if (!rhs.IsObject()) {
    stack->error->Report(Error::Type, "instanceof requires object");
    IV_LV5_BREAKER_RAISE();
  }
  JSObject* const robj = rhs.object();
  if (!robj->IsCallable()) {
    stack->error->Report(Error::Type, "instanceof requires constructor");
    IV_LV5_BREAKER_RAISE();
  }
  const bool result =
      static_cast<JSFunction*>(robj)->HasInstance(ctx, lhs, ERR);
  return Extract(JSVal::Bool(result));
}

inline Rep BINARY_IN(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  if (!rhs.IsObject()) {
    stack->error->Report(Error::Type, "in requires object");
    IV_LV5_BREAKER_RAISE();
  }
  const Symbol name = lhs.ToSymbol(ctx, ERR);
  return Extract(JSVal::Bool(rhs.object()->HasProperty(ctx, name)));
}

inline Rep BINARY_EQ(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const bool result = JSVal::AbstractEqual(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(result));
}

inline Rep BINARY_STRICT_EQ(JSVal lhs, JSVal rhs) {
  return Extract(JSVal::Bool(JSVal::StrictEqual(lhs, rhs)));
}

inline Rep BINARY_NE(Frame* stack, JSVal lhs, JSVal rhs) {
  const bool result = JSVal::AbstractEqual(stack->ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(!result));
}

inline Rep BINARY_STRICT_NE(JSVal lhs, JSVal rhs) {
  return Extract(JSVal::Bool(!JSVal::StrictEqual(lhs, rhs)));
}

inline Rep BINARY_BIT_AND(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left & right));
}

inline Rep BINARY_BIT_XOR(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left ^ right));
}

inline Rep BINARY_BIT_OR(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left | right));
}

inline Rep TO_NUMBER(Frame* stack, JSVal src) {
  const double x = src.ToNumber(stack->ctx, ERR);
  return Extract(x);
}

inline Rep UNARY_NEGATIVE(Frame* stack, JSVal src) {
  const double x = src.ToNumber(stack->ctx, ERR);
  return Extract(-x);
}

inline JSVal UNARY_NOT(JSVal src) {
  return JSVal::Bool(!src.ToBoolean());
}

inline Rep UNARY_BIT_NOT(Frame* stack, JSVal src) {
  const double value = src.ToNumber(stack->ctx, ERR);
  return Extract(JSVal::Int32(~core::DoubleToInt32(value)));
}

inline Rep THROW(Frame* stack, JSVal src) {
  stack->error->Report(src);
  IV_LV5_BREAKER_RAISE();
}

inline Rep THROW_WITH_TYPE_AND_MESSAGE(Frame* stack, Error::Code type, const char* message) {
  stack->error->Report(type, message);
  IV_LV5_BREAKER_RAISE();
}

inline void RaiseReferenceError(Symbol name, Error* e) {
  core::UStringBuilder builder;
  builder.Append('"');
  builder.Append(symbol::GetSymbolString(name));
  builder.Append("\" not defined");
  e->Report(Error::Reference, builder.BuildPiece());
}

inline JSEnv* GetEnv(Context* ctx, JSEnv* env, Symbol name) {
  JSEnv* current = env;
  while (current) {
    if (current->HasBinding(ctx, name)) {
      return current;
    } else {
      current = current->outer();
    }
  }
  return NULL;
}

template<bool Strict>
inline Rep LOAD_GLOBAL(Frame* stack, Symbol name, MonoIC* ic, Assembler* as) {
  // opcode | (dst | index) | nop | nop
  Context* ctx = stack->ctx;
  JSGlobal* global = ctx->global_obj();
  Slot slot;
  if (global->GetOwnPropertySlot(ctx, name, &slot)) {
    // now Own Property Pattern only implemented
    if (slot.IsLoadCacheable() && (slot.offset() * sizeof(JSVal)) <= MonoIC::kMaxOffset) {
      ic->Repatch(as, global->map(), slot.offset() * sizeof(JSVal));
      return Extract(slot.value());
    }
    const JSVal ret = slot.Get(ctx, global, ERR);
    return Extract(ret);
  } else {
    if (ctx->global_env()->HasBinding(ctx, name)) {
      const JSVal res = ctx->global_env()->GetBindingValue(ctx, name, Strict, ERR);
      return Extract(res);
    }
    RaiseReferenceError(name, stack->error);
    IV_LV5_BREAKER_RAISE();
    return 0;
  }
}

template<bool Strict>
inline Rep STORE_GLOBAL(Frame* stack, Symbol name,
                        MonoIC* ic, Assembler* as, JSVal src) {
  // opcode | (src | name) | nop | nop
  Context* ctx = stack->ctx;
  JSGlobal* global = ctx->global_obj();
  Slot slot;
  if (global->GetOwnPropertySlot(ctx, name, &slot)) {
    if (slot.IsStoreCacheable() && (slot.offset() * sizeof(JSVal)) <= MonoIC::kMaxOffset) {
      ic->Repatch(as, global->map(), slot.offset() * sizeof(JSVal));
      global->Direct(slot.offset()) = src;
    } else {
      global->Put(ctx, name, src, Strict, ERR);
    }
  } else {
    if (ctx->global_env()->HasBinding(ctx, name)) {
      ctx->global_env()->SetMutableBinding(ctx, name, src, Strict, ERR);
    } else {
      if (Strict) {
        stack->error->Report(Error::Reference,
                  "putting to unresolvable reference "
                  "not allowed in strict reference");
        IV_LV5_BREAKER_RAISE();
      } else {
        ctx->global_obj()->Put(ctx, name, src, Strict, ERR);
      }
    }
  }
  return 0;
}

inline Rep DELETE_GLOBAL(Frame* stack, Symbol name) {
  Context* ctx = stack->ctx;
  JSEnv* global = ctx->global_env();
  if (global->HasBinding(ctx, name)) {
    const bool res = global->DeleteBinding(ctx, name);
    return Extract(JSVal::Bool(res));
  } else {
    // not found -> unresolvable reference
    return Extract(JSTrue);
  }
}

template<bool Strict>
inline Rep TYPEOF_GLOBAL(Frame* stack, Symbol name) {
  Context* ctx = stack->ctx;
  JSEnv* global = ctx->global_env();
  if (global->HasBinding(ctx, name)) {
    const JSVal res = global->GetBindingValue(ctx, name, Strict, ERR);
    return Extract(res.TypeOf(ctx));
  } else {
    // not found -> unresolvable reference
    return Extract(ctx->global_data()->string_undefined());
  }
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_HEAP(Frame* stack, JSEnv* env, uint32_t offset) {
  JSDeclEnv* decl = static_cast<JSDeclEnv*>(env);
  const JSVal w = decl->GetByOffset(offset, Strict, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    decl->SetByOffset(offset, std::get<1>(results), Strict, ERR);
    return Extract(std::get<Returned>(results));
  } else {
    Context* ctx = stack->ctx;
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    decl->SetByOffset(offset, std::get<1>(results), Strict, ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

template<int Target, std::size_t Returned, bool Strict>
inline JSVal IncrementName(Context* ctx,
                           JSEnv* env, Symbol name, Error* e) {
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal w = current->GetBindingValue(ctx, name, Strict, IV_LV5_ERROR(e));
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
      IncrementName<Target, Returned, Strict>(stack->ctx, env, name, ERR);
  return Extract(res);
}

inline Rep CALL(Frame* stack,
                JSVal callee,
                JSVal* offset,
                uint64_t argc_with_this,
                railgun::Frame** out_frame,
                railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  if (!callee.IsCallable()) {
    stack->error->Report(Error::Type, "not callable object");
    IV_LV5_BREAKER_RAISE();
  }
  JSFunction* func =
      static_cast<JSFunction*>(callee.object());
  if (!func->IsNativeFunction()) {
    // inline call
    JSJITFunction* vm_func = static_cast<JSJITFunction*>(func);
    railgun::Code* code = vm_func->code();
    if (code->empty()) {
      return Extract(JSUndefined);
    }
    railgun::Frame* new_frame = ctx->vm()->stack()->NewCodeFrame(
        ctx,
        offset,
        code,
        vm_func->scope(),
        func,
        instr,
        argc_with_this, false);
    if (!new_frame) {
      stack->error->Report(Error::Range, "maximum call stack size exceeded");
      IV_LV5_BREAKER_RAISE();
    }
    new_frame->InitThisBinding(ctx);
    *out_frame = new_frame;
    return reinterpret_cast<Rep>(code->executable());
  }

  // Native Function
  {
    railgun::detail::VMArguments args(ctx,
                                      offset + (argc_with_this - 1),
                                      argc_with_this - 1);
    const JSVal res = func->Call(&args, args.this_binding(), ERR);
    return Extract(res);
  }
}

inline Rep EVAL(Frame* stack,
                JSVal callee,
                JSVal* offset,
                uint64_t argc_with_this,
                railgun::Frame** out_frame,
                railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  if (!callee.IsCallable()) {
    stack->error->Report(Error::Type, "not callable object");
    IV_LV5_BREAKER_RAISE();
  }
  JSFunction* func =
      static_cast<JSFunction*>(callee.object());
  if (!func->IsNativeFunction()) {
    // inline call
    JSJITFunction* vm_func = static_cast<JSJITFunction*>(func);
    railgun::Code* code = vm_func->code();
    if (code->empty()) {
      return Extract(JSUndefined);
    }
    railgun::Frame* new_frame = ctx->vm()->stack()->NewCodeFrame(
        ctx,
        offset,
        code,
        vm_func->scope(),
        func,
        instr,
        argc_with_this, false);
    if (!new_frame) {
      stack->error->Report(Error::Range, "maximum call stack size exceeded");
      IV_LV5_BREAKER_RAISE();
    }
    new_frame->InitThisBinding(ctx);
    *out_frame = new_frame;
    return reinterpret_cast<Rep>(code->executable());
  }

  // Native Function
  {
    railgun::detail::VMArguments args(ctx,
                                      offset + (argc_with_this - 1),
                                      argc_with_this - 1);
    const JSAPI native = func->NativeFunction();
    if (native && native == &GlobalEval) {
      // direct call to eval point
      args.set_this_binding(args.this_binding());
      const JSVal res = breaker::DirectCallToEval(args, *out_frame, ERR);
      return Extract(res);
    }
    const JSVal res = func->Call(&args, args.this_binding(), ERR);
    return Extract(res);
  }
}

inline Rep CONSTRUCT(Frame* stack,
                     JSVal callee,
                     JSVal* offset,
                     uint64_t argc_with_this,
                     railgun::Frame** out_frame,
                     railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  if (!callee.IsCallable()) {
    stack->error->Report(Error::Type, "not callable object");
    IV_LV5_BREAKER_RAISE();
  }
  JSFunction* func =
      static_cast<JSFunction*>(callee.object());
  if (!func->IsNativeFunction()) {
    // inline call
    JSJITFunction* vm_func = static_cast<JSJITFunction*>(func);
    railgun::Code* code = vm_func->code();
    railgun::Frame* new_frame = ctx->vm()->stack()->NewCodeFrame(
        ctx,
        offset,
        code,
        vm_func->scope(),
        func,
        instr,
        argc_with_this, false);
    if (!new_frame) {
      stack->error->Report(Error::Range, "maximum call stack size exceeded");
      IV_LV5_BREAKER_RAISE();
    }
    JSObject* const obj = JSObject::New(ctx, code->ConstructMap(ctx));
    new_frame->set_this_binding(obj);
    const JSVal proto = func->Get(ctx, symbol::prototype(), ERR);
    if (proto.IsObject()) {
      obj->set_prototype(proto.object());
    }
    new_frame->InitThisBinding(ctx);
    *out_frame = new_frame;
    return reinterpret_cast<Rep>(code->executable());
  }

  // Native Function
  {
    railgun::detail::VMArguments args(ctx,
                                      offset + (argc_with_this - 1),
                                      argc_with_this - 1);
    args.set_constructor_call(true);
    const JSVal res = func->Construct(&args, ERR);
    return Extract(res);
  }
}

inline Rep CONCAT(Frame* stack, JSVal* src, uint32_t count) {
  JSString* result = JSString::New(stack->ctx, src, count, ERR);
  return Extract(result);
}

inline Rep RAISE(Frame* stack, Error::Code code, JSString* str) {
  stack->error->Report(code, str->GetUString());
  IV_LV5_BREAKER_RAISE();
  return 0;
}

inline Rep TYPEOF(Context* ctx, JSVal src) {
  return Extract(src.TypeOf(ctx));
}

inline Rep TO_PRIMITIVE_AND_TO_STRING(Frame* stack, JSVal src) {
  Context* ctx = stack->ctx;
  const JSVal primitive = src.ToPrimitive(ctx, Hint::NONE, ERR);
  JSString* str = primitive.ToString(ctx, ERR);
  return Extract(str);
}

inline void STORE_OBJECT_GET(Context* ctx, JSVal target, JSVal item, uint32_t offset) {
  JSObject* obj = target.object();
  obj->Direct(offset) = JSVal::Cell(Accessor::New(ctx, item.object(), NULL));
}

inline void STORE_OBJECT_SET(Context* ctx, JSVal target, JSVal item, uint32_t offset) {
  JSObject* obj = target.object();
  obj->Direct(offset) = JSVal::Cell(Accessor::New(ctx, NULL, item.object()));
}

inline void INIT_VECTOR_ARRAY_ELEMENT(
    JSVal target, const JSVal* reg, uint32_t index, uint32_t size) {
  JSArray* ary = static_cast<JSArray*>(target.object());
  ary->SetToVector(index, reg, reg + size);
}

inline void INIT_SPARSE_ARRAY_ELEMENT(
    JSVal target, const JSVal* reg, uint32_t index, uint32_t size) {
  JSArray* ary = static_cast<JSArray*>(target.object());
  ary->SetToMap(index, reg, reg + size);
}

template<bool CONFIGURABLE>
inline Rep INSTANTIATE_DECLARATION_BINDING(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (!env->HasBinding(ctx, name)) {
    env->CreateMutableBinding(ctx, name, CONFIGURABLE, ERR);
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
          true, ERR);
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
    env->CreateMutableBinding(ctx, name, CONFIGURABLE, ERR);
    env->SetMutableBinding(ctx, name, JSUndefined, Strict, ERR);
  }
  return 0;
}

inline void INITIALIZE_HEAP_IMMUTABLE(JSEnv* env, JSVal src, uint32_t offset) {
  static_cast<JSDeclEnv*>(env)->InitializeImmutableBinding(offset, src);
}

inline Rep INCREMENT(Frame* stack, JSVal src) {
  const double res = src.ToNumber(stack->ctx, ERR);
  return Extract(res + 1);
}


inline Rep DECREMENT(Frame* stack, JSVal src) {
  const double res = src.ToNumber(stack->ctx, ERR);
  return Extract(res - 1);
}

inline Rep POSTFIX_INCREMENT(Frame* stack, JSVal val, JSVal* src) {
  const double res = val.ToNumber(stack->ctx, ERR);
  *src = res;
  return Extract(res + 1);
}

inline Rep POSTFIX_DECREMENT(Frame* stack,
                             JSVal val, JSVal* src) {
  const double res = val.ToNumber(stack->ctx, ERR);
  *src = res;
  return Extract(res - 1);
}

inline bool TO_BOOLEAN(JSVal src) {
  return src.ToBoolean();
}

template<bool Strict>
inline Rep LOAD_ARGUMENTS(Frame* stack, railgun::Frame* frame) {
  if (Strict) {
    JSObject* obj = JSStrictArguments::New(
        stack->ctx,
        static_cast<JSFunction*>(frame->callee().object()),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        ERR);
    return Extract(obj);
  } else {
    JSObject* obj = JSNormalArguments::New(
        stack->ctx,
        static_cast<JSFunction*>(frame->callee().object()),
        frame->code()->params(),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        static_cast<JSDeclEnv*>(frame->variable_env()),
        ERR);
    return Extract(obj);
  }
}

inline Rep PREPARE_DYNAMIC_CALL(Frame* stack,
                                JSEnv* env,
                                Symbol name,
                                JSVal* base) {
  Context* ctx = stack->ctx;
  if (JSEnv* target_env = GetEnv(ctx, env, name)) {
    const JSVal res = target_env->GetBindingValue(ctx, name, false, ERR);
    *base = target_env->ImplicitThisValue();
    return Extract(res);
  }
  RaiseReferenceError(name, stack->error);
  IV_LV5_BREAKER_RAISE();
}

template<bool Strict>
inline Rep LOAD_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal res = current->GetBindingValue(ctx, name, Strict, ERR);
    return Extract(res);
  }
  RaiseReferenceError(name, stack->error);
  IV_LV5_BREAKER_RAISE();
}

template<bool Strict>
inline Rep STORE_NAME(Frame* stack, JSEnv* env, Symbol name, JSVal src) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    current->SetMutableBinding(ctx, name, src, Strict, ERR);
  } else {
    if (Strict) {
      stack->error->Report(Error::Reference,
                           "putting to unresolvable reference "
                           "not allowed in strict reference");
      IV_LV5_BREAKER_RAISE();
    } else {
      ctx->global_obj()->Put(ctx, name, src, Strict, ERR);
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
    const JSVal res = current->GetBindingValue(ctx, name, Strict, ERR);
    return Extract(res.TypeOf(ctx));
  }
  return Extract(ctx->global_data()->string_undefined());
}

inline bool GetPrimitiveOwnProperty(Context* ctx,
                                    JSVal base, Symbol name, JSVal* res) {
  // section 8.7.1 special [[Get]]
  assert(base.IsPrimitive());
  if (base.IsString()) {
    // string short circuit
    JSString* str = base.string();
    if (name == symbol::length()) {
      *res = JSVal::UInt32(static_cast<uint32_t>(str->size()));
      return true;
    }
    if (symbol::IsArrayIndexSymbol(name)) {
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      if (index < str->size()) {
        *res = JSString::NewSingle(ctx, str->At(index));
        return true;
      }
    }
  }
  return false;
}

inline JSVal LoadPropPrimitive(Context* ctx,
                               JSVal base, Symbol name, Error* e) {
  JSVal res;
  if (GetPrimitiveOwnProperty(ctx, base, name, &res)) {
    return res;
  }
  // if base is primitive, property not found in "this" object
  // so, lookup from proto
  Slot slot;
  JSObject* const proto = base.GetPrimitiveProto(ctx);
  if (proto->GetPropertySlot(ctx, name, &slot)) {
    return slot.Get(ctx, base, e);
  } else {
    return JSUndefined;
  }
}

inline Rep LOAD_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  Slot slot;
  const Symbol name = element.ToSymbol(ctx, ERR);
  const JSVal res = base.GetSlot(ctx, name, &slot, ERR);
  return Extract(res);
}

template<bool Strict>
void StorePropPrimitive(Context* ctx,
                        JSVal base, Symbol name, JSVal stored, Error* e) {
  assert(base.IsPrimitive());
  JSObject* const o = base.ToObject(ctx, IV_LV5_ERROR_VOID(e));
  if (!o->CanPut(ctx, name)) {
    if (Strict) {
      e->Report(Error::Type, "cannot put value to object");
    }
    return;
  }
  const PropertyDescriptor own_desc = o->GetOwnProperty(ctx, name);
  if (!own_desc.IsEmpty() && own_desc.IsData()) {
    if (Strict) {
      e->Report(Error::Type,
                "value to symbol defined and not data descriptor");
    }
    return;
  }
  const PropertyDescriptor desc = o->GetProperty(ctx, name);
  if (!desc.IsEmpty() && desc.IsAccessor()) {
    ScopedArguments a(ctx, 1, IV_LV5_ERROR_VOID(e));
    a[0] = stored;
    const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
    assert(ac->set());
    static_cast<JSFunction*>(ac->set())->Call(&a, base, e);
    return;
  } else {
    if (Strict) {
      e->Report(Error::Type, "value to symbol in transient object");
    }
  }
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

template<bool Strict>
inline Rep STORE_ELEMENT(Frame* stack, JSVal base, JSVal src, JSVal element) {
  Context* ctx = stack->ctx;
  const Symbol name = element.ToSymbol(ctx, ERR);
  StorePropImpl<Strict>(ctx, base, name, src, ERR);
  return 0;
}

template<bool Strict>
inline Rep DELETE_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  uint32_t index;
  if (element.GetUInt32(&index)) {
    JSObject* const obj = base.ToObject(ctx, ERR);
    const bool result =
        obj->Delete(ctx, symbol::MakeSymbolFromIndex(index), Strict, ERR);
    return Extract(JSVal::Bool(result));
  } else {
    const Symbol name = element.ToSymbol(ctx, ERR);
    JSObject* const obj = base.ToObject(ctx, ERR);
    const bool result = obj->Delete(ctx, name, Strict, ERR);
    return Extract(JSVal::Bool(result));
  }
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  const Symbol name = element.ToSymbol(ctx, ERR);
  Slot slot;
  const JSVal w = base.GetSlot(ctx, name, &slot, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), ERR);
    return Extract(std::get<Returned>(results));
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

inline Rep LOAD_REGEXP(Context* ctx, JSRegExp* reg) {
  return Extract(JSRegExp::New(ctx, reg));
}

inline Rep LOAD_OBJECT(Context* ctx, Map* map) {
  return Extract(JSObject::New(ctx, map));
}

inline Rep LOAD_ARRAY(Context* ctx, uint32_t len) {
  return Extract(JSArray::ReservedNew(ctx, len));
}

inline Rep DUP_ARRAY(Context* ctx, const JSVal constant) {
  return Extract(JSArray::New(ctx, static_cast<JSArray*>(constant.object())));
}

inline JSEnv* TRY_CATCH_SETUP(Context* ctx,
                              JSEnv* outer, Symbol sym, JSVal value) {
  return JSStaticEnv::New(ctx, outer, sym, value);
}

inline Rep STORE_PROP_GENERIC(Frame* stack, JSVal base, JSVal src, StorePropertyIC* ic) {
  if (ic->strict()) {
    StorePropImpl<true>(stack->ctx, base, ic->name(), src, ERR);
  } else {
    StorePropImpl<false>(stack->ctx, base, ic->name(), src, ERR);
  }
  return 0;
}

inline Rep LOAD_PROP_GENERIC(Frame* stack, JSVal base, LoadPropertyIC* site) {  // NOLINT
  Slot slot;
  const JSVal res = base.GetSlot(stack->ctx, site->name(), &slot, ERR);
  return Extract(res);
}

inline Rep LOAD_PROP(Frame* stack, JSVal base, LoadPropertyIC* site) {  // NOLINT
  assert(!symbol::IsArrayIndexSymbol(site->name()));
  Context* ctx = stack->ctx;
  Slot slot;
  const Symbol name = site->name();

  // String / Array length fast path
  if (name == symbol::length()) {
    if (base.IsString()) {
      site->LoadStringLength(ctx);
      return Extract(JSVal::UInt32(base.string()->size()));
    } else if (base.IsObject() && base.object()->IsClass<Class::Array>()) {
      site->LoadArrayLength(ctx);
      return Extract(JSVal::UInt32(static_cast<JSArray*>(base.object())->GetLength()));
    }
  }

  const JSVal res = base.GetSlot(ctx, name, &slot, ERR);

  if (slot.IsNotFound()) {
    return Extract(res);
  }

  // property found

  // uncacheable path
  if (!slot.IsLoadCacheable() || !base.IsCell()) {
    // bailout to generic
    // TODO(Constellation) insert generic IC
    return Extract(res);
  }

  JSObject* obj = NULL;
  if (base.IsString()) {
    obj = ctx->global_data()->string_prototype();
  } else {
    obj = base.object();
  }

  // cache phase
  // own property / proto property / chain lookup property
  if (slot.base() == obj) {
    // own property
    site->LoadOwnProperty(ctx, obj->map(), slot.offset());
    return Extract(res);
  }

  if (slot.base() == obj->prototype()) {
    // proto property
    obj->FlattenMap();
    site->LoadPrototypeProperty(ctx, obj->map(), slot.base()->map(), slot.offset());
    return Extract(res);
  }

  // chain property
  site->LoadChainProperty(ctx, Chain::New(obj, slot.base()), slot.base()->map(), slot.offset());
  return Extract(res);
}

inline Rep STORE_PROP(Frame* stack, JSVal base, JSVal src, StorePropertyIC* ic) {
  Context* ctx = stack->ctx;
  const Symbol name = ic->name();
  if (base.IsPrimitive()) {
    if (ic->strict()) {
      StorePropPrimitive<true>(ctx, base, name, src, ERR);
    } else {
      StorePropPrimitive<false>(ctx, base, name, src, ERR);
    }
    return 0;
  }

  JSObject* obj = base.object();
  Slot slot;
  if (!obj->GetOwnPropertySlot(ctx, name, &slot)) {
    // new property
    obj->Put(ctx, name, src, ic->strict(), ERR);
    return 0;
  }

  if (!slot.IsStoreCacheable()) {
    obj->Put(ctx, name, src, ic->strict(), ERR);
    return 0;
  }

  // cache, replace property
  ic->StoreReplaceProperty(obj->map(), slot.offset());
  obj->Direct(slot.offset()) = src;
  return 0;
}

template<bool Strict>
inline Rep DELETE_PROP(Frame* stack, JSVal base, Symbol name) {
  Context* ctx = stack->ctx;
  JSObject* const obj = base.ToObject(ctx, ERR);
  const bool res = obj->Delete(ctx, name, Strict, ERR);
  return Extract(JSVal::Bool(res));
}

template<int Target, std::size_t Returned, bool Strict>
inline Rep INCREMENT_PROP(Frame* stack, JSVal base, Symbol name) {
  Context* ctx = stack->ctx;
  Slot slot;
  const JSVal w = base.GetSlot(ctx, name, &slot, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), ERR);
    return Extract(std::get<Returned>(results));
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl<Strict>(ctx, base, name, std::get<1>(results), ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

#undef ERR
#undef IV_LV5_BREAKER_RAISE
} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_STUB_H_
