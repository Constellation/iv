// Stub functions implementations
#ifndef IV_LV5_BREAKER_STUB_H_
#define IV_LV5_BREAKER_STUB_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/templates.h>
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/jsfunction.h>
#include <iv/lv5/breaker/context.h>
#include <iv/lv5/breaker/mono_ic.h>
#include <iv/lv5/accessor.h>
namespace iv {
namespace lv5 {
namespace breaker {

inline Rep Extract(JSVal val) {
  return val.Layout().bytes_;
}

namespace stub {

#define RAISE()\
  do {\
    void* pc = *stack->ret;\
    *stack->ret = Templates<>::dispatch_exception_handler();  /* NOLINT */\
    return core::BitCast<uint64_t>(pc);\
  } while (0)

#define ERR\
  stack->error);\
  do {\
    if (*stack->error) {\
      RAISE();\
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
      return Extract(JSString::New(ctx, lhs.string(), rhs.string()));
    } else {
      const JSVal rp = rhs.ToPrimitive(ctx, Hint::NONE, ERR);
      JSString* const rs = rp.ToString(ctx, ERR);
      return Extract(JSString::New(ctx, lhs.string(), rs));
    }
  }

  const JSVal lprim = lhs.ToPrimitive(ctx, Hint::NONE, ERR);
  const JSVal rprim = rhs.ToPrimitive(ctx, Hint::NONE, ERR);
  if (lprim.IsString() || rprim.IsString()) {
    JSString* const lstr = lprim.ToString(ctx, ERR);
    JSString* const rstr = rprim.ToString(ctx, ERR);
    return Extract(JSString::New(ctx, lstr, rstr));
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
    RAISE();
  }
  JSObject* const robj = rhs.object();
  if (!robj->IsCallable()) {
    stack->error->Report(Error::Type, "instanceof requires constructor");
    RAISE();
  }
  const bool result = robj->AsCallable()->HasInstance(ctx, lhs, ERR);
  return Extract(JSVal::Bool(result));
}

inline Rep BINARY_IN(Frame* stack, JSVal lhs, JSVal rhs) {
  Context* ctx = stack->ctx;
  if (!rhs.IsObject()) {
    stack->error->Report(Error::Type, "in requires object");
    RAISE();
  }
  const Symbol s = lhs.ToSymbol(ctx, ERR);
  return Extract(JSVal::Bool(rhs.object()->HasProperty(ctx, s)));
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
  RAISE();
}

inline Rep THROW_WITH_TYPE_AND_MESSAGE(Frame* stack, Error::Code type, const char* message) {
  stack->error->Report(type, message);
  RAISE();
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

template<bool STRICT>
inline Rep LOAD_GLOBAL(Frame* stack, Symbol name, MonoIC* ic, Assembler* as) {
  // opcode | (dst | index) | nop | nop
  Context* ctx = stack->ctx;
  JSGlobal* global = ctx->global_obj();
  Slot slot;
  if (global->GetOwnPropertySlot(ctx, name, &slot)) {
    // now Own Property Pattern only implemented
    if (slot.IsLoadCacheable()) {
      ic->LoadRepatch(as, global->map(), slot.offset());
      return Extract(slot.value());
    }
    const JSVal ret = slot.Get(ctx, global, ERR);
    return Extract(ret);
  } else {
    if (ctx->global_env()->HasBinding(ctx, name)) {
      const JSVal res = ctx->global_env()->GetBindingValue(ctx, name, STRICT, ERR);
      return Extract(res);
    }
    RaiseReferenceError(name, stack->error);
    RAISE();
    return 0;
  }
}

template<bool STRICT>
inline Rep STORE_GLOBAL(Frame* stack,
                        JSVal src,
                        Symbol name, railgun::Instruction* instr) {
  // opcode | (src | name) | nop | nop
  Context* ctx = stack->ctx;
  JSGlobal* global = ctx->global_obj();
  if (instr[2].map == global->map()) {
    // map is cached, so use previous index code
    global->GetSlot(instr[3].u32[0]) = src;
  } else {
    Slot slot;
    if (global->GetOwnPropertySlot(ctx, name, &slot)) {
      if (slot.IsStoreCacheable()) {
        instr[2].map = global->map();
        instr[3].u32[0] = slot.offset();
        global->GetSlot(slot.offset()) = src;
      } else {
        global->Put(ctx, name, src, STRICT, ERR);
      }
    } else {
      instr[2].map = NULL;
      if (JSEnv* current = GetEnv(ctx, ctx->global_env(), name)) {
        current->SetMutableBinding(ctx, name, src, STRICT, ERR);
      } else {
        if (STRICT) {
          stack->error->Report(Error::Reference,
                    "putting to unresolvable reference "
                    "not allowed in strict reference");
          RAISE();
        } else {
          ctx->global_obj()->Put(ctx, name, src, STRICT, ERR);
        }
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

template<bool STRICT>
inline Rep TYPEOF_GLOBAL(Frame* stack, Symbol name) {
  Context* ctx = stack->ctx;
  JSEnv* global = ctx->global_env();
  if (global->HasBinding(ctx, name)) {
    const JSVal res = global->GetBindingValue(ctx, name, STRICT, ERR);
    return Extract(res.TypeOf(ctx));
  } else {
    // not found -> unresolvable reference
    return Extract(ctx->global_data()->string_undefined());
  }
}

template<int Target, std::size_t Returned, bool STRICT>
inline Rep INCREMENT_HEAP(Frame* stack, JSEnv* env, uint32_t offset) {
  JSDeclEnv* decl = static_cast<JSDeclEnv*>(env);
  const JSVal w = decl->GetByOffset(offset, STRICT, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    decl->SetByOffset(offset, std::get<1>(results), STRICT, ERR);
    return Extract(std::get<Returned>(results));
  } else {
    Context* ctx = stack->ctx;
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    decl->SetByOffset(offset, std::get<1>(results), STRICT, ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

template<int Target, std::size_t Returned, bool STRICT>
inline JSVal IncrementName(Context* ctx,
                           JSEnv* env, Symbol s, Error* e) {
  if (JSEnv* current = GetEnv(ctx, env, s)) {
    const JSVal w = current->GetBindingValue(ctx, s, STRICT, IV_LV5_ERROR(e));
    if (w.IsInt32() &&
        railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
      std::tuple<JSVal, JSVal> results;
      const int32_t target = w.int32();
      std::get<0>(results) = w;
      std::get<1>(results) = JSVal::Int32(target + Target);
      current->SetMutableBinding(ctx, s, std::get<1>(results), STRICT, e);
      return std::get<Returned>(results);
    } else {
      std::tuple<double, double> results;
      std::get<0>(results) = w.ToNumber(ctx, IV_LV5_ERROR(e));
      std::get<1>(results) = std::get<0>(results) + Target;
      current->SetMutableBinding(ctx, s, std::get<1>(results), STRICT, e);
      return std::get<Returned>(results);
    }
  }
  RaiseReferenceError(s, e);
  return 0.0;
}

template<int Target, std::size_t Returned, bool STRICT>
inline Rep INCREMENT_NAME(Frame* stack, JSEnv* env, Symbol name) {
  const JSVal res =
      IncrementName<Target, Returned, STRICT>(stack->ctx, env, name, ERR);
  return Extract(res);
}

template<int Target, std::size_t Returned, bool STRICT>
inline JSVal IncrementGlobal(Context* ctx,
                             JSGlobal* global,
                             uint32_t offset, Error* e) {
  const JSVal w = global->GetSlot(offset);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    global->GetSlot(offset) = std::get<1>(results);
    return std::get<Returned>(results);
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, IV_LV5_ERROR(e));
    std::get<1>(results) = std::get<0>(results) + Target;
    global->GetSlot(offset) = std::get<1>(results);
    return std::get<Returned>(results);
  }
}

template<int Target, std::size_t Returned, bool STRICT>
inline Rep INCREMENT_GLOBAL(Frame* stack,
                            railgun::Instruction* instr, Symbol s) {
  // opcode | (dst | name) | nop | nop
  Context* ctx = stack->ctx;
  JSGlobal* global = ctx->global_obj();
  if (instr[2].map == global->map()) {
    // map is cached, so use previous index code
    const JSVal res =
        IncrementGlobal<Target,
                        Returned,
                        STRICT>(ctx, global, instr[3].u32[0], ERR);
    return Extract(res);
  } else {
    Slot slot;
    if (global->GetOwnPropertySlot(ctx, s, &slot) && slot.IsStoreCacheable()) {
      instr[2].map = global->map();
      instr[3].u32[0] = slot.offset();
      const JSVal res =
          IncrementGlobal<Target,
                          Returned,
                          STRICT>(ctx, global, instr[3].u32[0], ERR);
      return Extract(res);
    } else {
      instr[2].map = NULL;
      const JSVal res =
          IncrementName<Target,
                        Returned,
                        STRICT>(ctx, ctx->global_env(), s, ERR);
      return Extract(res);
    }
  }
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
    RAISE();
  }
  lv5::JSFunction* func = callee.object()->AsCallable();
  if (!func->IsNativeFunction()) {
    // inline call
    JSFunction* vm_func = static_cast<JSFunction*>(func);
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
      RAISE();
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
    RAISE();
  }
  lv5::JSFunction* func = callee.object()->AsCallable();
  if (!func->IsNativeFunction()) {
    // inline call
    JSFunction* vm_func = static_cast<JSFunction*>(func);
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
      RAISE();
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
    RAISE();
  }
  lv5::JSFunction* func = callee.object()->AsCallable();
  if (!func->IsNativeFunction()) {
    // inline call
    JSFunction* vm_func = static_cast<JSFunction*>(func);
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
      RAISE();
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

inline Rep CONCAT(Context* ctx, JSVal* src, uint32_t count) {
  return Extract(JSString::New(ctx, src, count));
}

inline Rep RAISE_REFERENCE(Frame* stack) {
  core::UStringBuilder builder;
  builder.Append("Invalid left-hand side expression");
  stack->error->Report(Error::Reference, builder.BuildPiece());
  RAISE();
  return 0;
}

inline Rep RAISE_IMMUTABLE(Frame* stack, Symbol name) {
  core::UStringBuilder builder;
  builder.Append("mutating immutable binding \"");
  builder.Append(symbol::GetSymbolString(name));
  builder.Append("\" not allowed in strict mode");
  stack->error->Report(Error::Type, builder.BuildPiece());
  RAISE();
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
  obj->GetSlot(offset) = JSVal::Cell(Accessor::New(ctx, item.object(), NULL));
}

inline void STORE_OBJECT_SET(Context* ctx, JSVal target, JSVal item, uint32_t offset) {
  JSObject* obj = target.object();
  obj->GetSlot(offset) = JSVal::Cell(Accessor::New(ctx, NULL, item.object()));
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
        RAISE();
      }
      const DataDescriptor* const data = existing_prop.AsDataDescriptor();
      if (!data->IsWritable() || !data->IsEnumerable()) {
        stack->error->Report(Error::Type,
                             "create mutable function binding failed");
        RAISE();
      }
    }
  }
  return 0;
}

template<bool CONFIGURABLE, bool STRICT>
inline Rep INSTANTIATE_VARIABLE_BINDING(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (!env->HasBinding(ctx, name)) {
    env->CreateMutableBinding(ctx, name, CONFIGURABLE, ERR);
    env->SetMutableBinding(ctx, name, JSUndefined, STRICT, ERR);
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

template<bool STRICT>
inline Rep LOAD_ARGUMENTS(Frame* stack, railgun::Frame* frame) {
  if (STRICT) {
    JSObject* obj = JSStrictArguments::New(
        stack->ctx, frame->callee().object()->AsCallable(),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        ERR);
    return Extract(obj);
  } else {
    JSObject* obj = JSNormalArguments::New(
        stack->ctx, frame->callee().object()->AsCallable(),
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
  RAISE();
}

template<bool STRICT>
inline Rep LOAD_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal res = current->GetBindingValue(ctx, name, STRICT, ERR);
    return Extract(res);
  }
  RaiseReferenceError(name, stack->error);
  RAISE();
}

template<bool STRICT>
inline Rep STORE_NAME(Frame* stack, JSEnv* env, Symbol name, JSVal src) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    current->SetMutableBinding(ctx, name, src, STRICT, ERR);
  } else {
    if (STRICT) {
      stack->error->Report(Error::Reference,
                           "putting to unresolvable reference "
                           "not allowed in strict reference");
      RAISE();
    } else {
      ctx->global_obj()->Put(ctx, name, src, STRICT, ERR);
    }
  }
  return 0;
}

template<bool STRICT>
inline Rep DELETE_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    return Extract(JSVal::Bool(current->DeleteBinding(ctx, name)));
  }
  return Extract(JSTrue);
}

template<bool STRICT>
inline Rep TYPEOF_NAME(Frame* stack, JSEnv* env, Symbol name) {
  Context* ctx = stack->ctx;
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal res = current->GetBindingValue(ctx, name, STRICT, ERR);
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

inline JSVal LoadPropImpl(Context* ctx, JSVal base, Symbol name, Error* e) {
  if (base.IsPrimitive()) {
    return LoadPropPrimitive(ctx, base, name, e);
  } else {
    return base.object()->Get(ctx, name, e);
  }
}

inline Rep LOAD_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  const Symbol name = element.ToSymbol(ctx, ERR);
  const JSVal res = LoadPropImpl(ctx, base, name, ERR);
  return Extract(res);
}

template<bool STRICT>
void StorePropPrimitive(Context* ctx,
                        JSVal base, Symbol name, JSVal stored, Error* e) {
  assert(base.IsPrimitive());
  JSObject* const o = base.ToObject(ctx, IV_LV5_ERROR_VOID(e));
  if (!o->CanPut(ctx, name)) {
    if (STRICT) {
      e->Report(Error::Type, "cannot put value to object");
    }
    return;
  }
  const PropertyDescriptor own_desc = o->GetOwnProperty(ctx, name);
  if (!own_desc.IsEmpty() && own_desc.IsData()) {
    if (STRICT) {
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
    ac->set()->AsCallable()->Call(&a, base, e);
    return;
  } else {
    if (STRICT) {
      e->Report(Error::Type, "value to symbol in transient object");
    }
  }
}

template<bool STRICT>
inline void StorePropImpl(Context* ctx,
                          JSVal base, Symbol name, JSVal stored, Error* e) {
  if (base.IsPrimitive()) {
    StorePropPrimitive<STRICT>(ctx, base, name, stored, e);
  } else {
    base.object()->Put(ctx, name, stored, STRICT, e);
  }
}

template<bool STRICT>
inline Rep STORE_ELEMENT(Frame* stack, JSVal base, JSVal element, JSVal src) {
  Context* ctx = stack->ctx;
  const Symbol name = element.ToSymbol(ctx, ERR);
  StorePropImpl<STRICT>(ctx, base, name, src, ERR);
  return 0;
}

template<bool STRICT>
inline Rep DELETE_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  uint32_t index;
  if (element.GetUInt32(&index)) {
    JSObject* const obj = base.ToObject(ctx, ERR);
    const bool result =
        obj->Delete(ctx, symbol::MakeSymbolFromIndex(index), STRICT, ERR);
    return Extract(JSVal::Bool(result));
  } else {
    const Symbol name = element.ToSymbol(ctx, ERR);
    JSObject* const obj = base.ToObject(ctx, ERR);
    const bool result = obj->Delete(ctx, name, STRICT, ERR);
    return Extract(JSVal::Bool(result));
  }
}

template<int Target, std::size_t Returned, bool STRICT>
inline Rep INCREMENT_ELEMENT(Frame* stack, JSVal base, JSVal element) {
  Context* ctx = stack->ctx;
  const Symbol s = element.ToSymbol(ctx, ERR);
  const JSVal w = LoadPropImpl(ctx, base, s, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    StorePropImpl<STRICT>(ctx, base, s, std::get<1>(results), ERR);
    return Extract(std::get<Returned>(results));
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl<STRICT>(ctx, base, s, std::get<1>(results), ERR);
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

template<bool STRICT>
inline Rep STORE_PROP_GENERIC(Frame* stack,
                              JSVal base, Symbol name, JSVal src,
                              railgun::Instruction* instr) {
  StorePropImpl<STRICT>(stack->ctx, base, name, src, ERR);
  return 0;
}

template<bool STRICT>
Rep LOAD_PROP(Frame* stack,
              JSVal base, Symbol name,
              railgun::Instruction* instr);

template<bool STRICT>
inline Rep LOAD_PROP_GENERIC(Frame* stack,
                             JSVal base, Symbol name,
                             railgun::Instruction* instr) {
  const JSVal res = LoadPropImpl(stack->ctx, base, name, ERR);
  return Extract(res);
}

template<bool STRICT>
inline Rep LOAD_PROP_OWN(Frame* stack,
                         JSVal base, Symbol name,
                         railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  JSObject* obj = NULL;
  if (base.IsPrimitive()) {
    // primitive prototype cache
    JSVal res;
    if (GetPrimitiveOwnProperty(ctx, base, name, &res)) {
      return Extract(res);
    } else {
      obj = base.GetPrimitiveProto(ctx);
    }
  } else {
    obj = base.object();
  }
  assert(obj);
  if (instr[2].map == obj->map()) {
    // cache hit
    return Extract(obj->GetSlot(instr[3].u32[0]));
  } else {
    // not found => uncache
    Slot slot;
    if (obj->GetPropertySlot(ctx, name, &slot)) {
      instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP);
      Assembler::RepatchSite::RepatchAfterCall(
          stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP<STRICT>));
      const JSVal ret = slot.Get(ctx, obj, ERR);
      return Extract(ret);
    } else {
      instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP);
      Assembler::RepatchSite::RepatchAfterCall(
          stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP<STRICT>));
      return Extract(JSUndefined);
    }
  }
}

template<bool STRICT>
inline Rep LOAD_PROP_PROTO(Frame* stack,
                           JSVal base, Symbol name,
                           railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  JSObject* obj = NULL;
  if (base.IsPrimitive()) {
    // primitive prototype cache
    JSVal res;
    if (GetPrimitiveOwnProperty(ctx, base, name, &res)) {
      return Extract(res);
    } else {
      obj = base.GetPrimitiveProto(ctx);
    }
  } else {
    obj = base.object();
  }
  JSObject* proto = obj->prototype();
  if (instr[2].map == obj->map() && proto && instr[3].map == proto->map()) {
    // cache hit
    return Extract(proto->GetSlot(instr[4].u32[0]));
  } else {
    // uncache
    instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP);
    Assembler::RepatchSite::RepatchAfterCall(
        stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP<STRICT>));
    const JSVal res = LoadPropImpl(ctx, base, name, ERR);
    return Extract(res);
  }
}

template<bool STRICT>
inline Rep LOAD_PROP_CHAIN(Frame* stack,
                           JSVal base, Symbol name,
                           railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  JSObject* obj = NULL;
  if (base.IsPrimitive()) {
    // primitive prototype cache
    JSVal res;
    if (GetPrimitiveOwnProperty(ctx, base, name, &res)) {
      return Extract(res);
    } else {
      obj = base.GetPrimitiveProto(ctx);
    }
  } else {
    obj = base.object();
  }
  if (JSObject* cached = instr[2].chain->Validate(obj, instr[3].map)) {
    // cache hit
    return Extract(cached->GetSlot(instr[4].u32[0]));
  } else {
    // uncache
    instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP);
    Assembler::RepatchSite::RepatchAfterCall(
        stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP<STRICT>));
    const JSVal res = LoadPropImpl(ctx, base, name, ERR);
    return Extract(res);
  }
}

template<bool STRICT>
inline Rep LOAD_PROP(Frame* stack,
                     JSVal base, Symbol name,
                     railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  JSObject* obj = NULL;
  if (base.IsPrimitive()) {
    JSVal res;
    if (GetPrimitiveOwnProperty(ctx, base, name, &res)) {
      return Extract(res);
    } else {
      // if base is primitive, property not found in "this" object
      // so, lookup from proto
      obj = base.GetPrimitiveProto(ctx);
    }
  } else {
    obj = base.object();
  }

  Slot slot;
  if (obj->GetPropertySlot(ctx, name, &slot)) {
    // property found
    if (!slot.IsLoadCacheable()) {
      // bailout to generic
      instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP_GENERIC);
      Assembler::RepatchSite::RepatchAfterCall(
          stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP_GENERIC<STRICT>));
      const JSVal res = slot.Get(ctx, base, ERR);
      return Extract(res);
    }

    // cache phase
    // own property / proto property / chain lookup property
    if (slot.base() == obj) {
      // own property
      instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP_OWN);
      Assembler::RepatchSite::RepatchAfterCall(
          stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP_OWN<STRICT>));
      instr[2].map = obj->map();
      instr[3].u32[0] = slot.offset();
      return Extract(slot.value());
    }

    if (slot.base() == obj->prototype()) {
      // proto property
      obj->FlattenMap();
      instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP_PROTO);
      Assembler::RepatchSite::RepatchAfterCall(
          stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP_PROTO<STRICT>));
      instr[2].map = obj->map();
      instr[3].map = slot.base()->map();
      instr[4].u32[0] = slot.offset();
      return Extract(slot.value());
    }

    // chain property
    instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::LOAD_PROP_CHAIN);
    Assembler::RepatchSite::RepatchAfterCall(
        stack->ret, core::BitCast<uint64_t>(&stub::LOAD_PROP_CHAIN<STRICT>));
    instr[2].chain = Chain::New(obj, slot.base());
    instr[3].map = slot.base()->map();
    instr[4].u32[0] = slot.offset();
    return Extract(slot.value());
  }
  return Extract(JSUndefined);
}

template<bool STRICT>
inline Rep STORE_PROP(Frame* stack,
                      JSVal base, Symbol name, JSVal src,
                      railgun::Instruction* instr) {
  Context* ctx = stack->ctx;
  if (base.IsPrimitive()) {
    StorePropPrimitive<STRICT>(ctx, base, name, src, ERR);
  } else {
    // cache patten
    JSObject* obj = base.object();
    if (instr[2].map == obj->map()) {
      // map is cached, so use previous index code
      obj->GetSlot(instr[3].u32[0]) = src;
    } else {
      Slot slot;
      if (obj->GetOwnPropertySlot(ctx, name, &slot)) {
        if (slot.IsStoreCacheable()) {
          instr[2].map = obj->map();
          instr[3].u32[0] = slot.offset();
          obj->GetSlot(slot.offset()) = src;
        } else {
          // dispatch generic path
          obj->Put(ctx, name, src, STRICT, ERR);
          instr[0] = railgun::Instruction::GetOPInstruction(railgun::OP::STORE_PROP_GENERIC);
          Assembler::RepatchSite::RepatchAfterCall(
              stack->ret, core::BitCast<uint64_t>(&stub::STORE_PROP_GENERIC<STRICT>));
        }
      } else {
        instr[2].map = NULL;
        obj->Put(ctx, name, src, STRICT, ERR);
      }
    }
  }
  return 0;
}

template<bool STRICT>
inline Rep DELETE_PROP(Frame* stack, JSVal base, Symbol name) {
  Context* ctx = stack->ctx;
  JSObject* const obj = base.ToObject(ctx, ERR);
  const bool res = obj->Delete(ctx, name, STRICT, ERR);
  return Extract(JSVal::Bool(res));
}

template<int Target, std::size_t Returned, bool STRICT>
inline Rep INCREMENT_PROP(Frame* stack, JSVal base, Symbol name) {
  Context* ctx = stack->ctx;
  const JSVal w = LoadPropImpl(ctx, base, name, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    StorePropImpl<STRICT>(ctx, base, name, std::get<1>(results), ERR);
    return Extract(std::get<Returned>(results));
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    StorePropImpl<STRICT>(ctx, base, name, std::get<1>(results), ERR);
    return Extract(JSVal(std::get<Returned>(results)));
  }
}

#undef ERR
#undef RAISE
} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_STUB_H_
