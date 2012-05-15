// Stub functions implementations
#ifndef IV_LV5_BREAKER_STUB_H_
#define IV_LV5_BREAKER_STUB_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/templates.h>
namespace iv {
namespace lv5 {
namespace breaker {

inline Rep Extract(JSVal val) {
  return val.Layout().bytes_;
}

namespace stub {

#define ERR\
  ctx->PendingError());\
  if (*ctx->PendingError()) {\
    IV_LV5_BREAKER_RAISE();\
  }\
((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY


inline void BUILD_ENV(railgun::Context* ctx, railgun::Frame* frame,
                      uint32_t size, uint32_t mutable_start) {
  frame->variable_env_ = frame->lexical_env_ =
      JSDeclEnv::New(ctx,
                     frame->lexical_env(),
                     size,
                     frame->code()->names().begin(),
                     mutable_start);
}

inline Rep WITH_SETUP(railgun::Context* ctx, railgun::Frame* frame, JSVal src) {
  JSObject* const obj = src.ToObject(ctx, ERR);
  JSObjectEnv* const with_env =
      JSObjectEnv::New(ctx, frame->lexical_env(), obj);
  with_env->set_provide_this(true);
  frame->set_lexical_env(with_env);
  return 0;
}

inline Rep FORIN_SETUP(railgun::Context* ctx, JSVal enumerable) {
  railgun::NativeIterator* it;
  if (enumerable.IsString()) {
    it = ctx->GainNativeIterator(enumerable.string());
  } else {
    JSObject* const obj = enumerable.ToObject(ctx, ERR);
    it = ctx->GainNativeIterator(obj);
  }
  return Extract(JSVal::Cell(it));
}

inline Rep FORIN_ENUMERATE(railgun::Context* ctx, JSVal iterator) {
  railgun::NativeIterator* it =
      static_cast<railgun::NativeIterator*>(iterator.cell());
  if (it->Has()) {
    const Symbol sym = it->Get();
    it->Next();
    return Extract(JSString::New(ctx, sym));
  }
  return 0;
}

inline void FORIN_LEAVE(railgun::Context* ctx, JSVal iterator) {
  railgun::NativeIterator* it =
      static_cast<railgun::NativeIterator*>(iterator.cell());
  ctx->ReleaseNativeIterator(it);
}

inline Rep BINARY_ADD(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  assert(!lhs.IsNumber() || !rhs.IsNumber());
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

inline Rep BINARY_SUBTRACT(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const double left = lhs.ToNumber(ctx, ERR);
  const double res = left -  rhs.ToNumber(ctx, ERR);
  return Extract(res);
}

inline Rep BINARY_MULTIPLY(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const double left = lhs.ToNumber(ctx, ERR);
  const double res = left *  rhs.ToNumber(ctx, ERR);
  return Extract(res);
}

inline Rep BINARY_DIVIDE(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const double left = lhs.ToNumber(ctx, ERR);
  const double res = left / rhs.ToNumber(ctx, ERR);
  return Extract(res);
}

inline Rep BINARY_MODULO(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const double left = lhs.ToNumber(ctx, ERR);
  const double right = rhs.ToNumber(ctx, ERR);
  return Extract(core::math::Modulo(left, right));
}

inline Rep BINARY_LSHIFT(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left << (right & 0x1f)));
}

inline Rep BINARY_RSHIFT(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left >> (right & 0x1f)));
}

inline Rep BINARY_RSHIFT_LOGICAL(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const uint32_t left = lhs.ToUInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::UInt32(left >> (right & 0x1f)));
}

inline Rep BINARY_LT(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const CompareResult res = JSVal::Compare<true>(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(res == CMP_TRUE));
}

inline Rep BINARY_LTE(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const CompareResult res = JSVal::Compare<false>(ctx, rhs, lhs, ERR);
  return Extract(JSVal::Bool(res == CMP_FALSE));
}

inline Rep BINARY_GT(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const CompareResult res = JSVal::Compare<false>(ctx, rhs, lhs, ERR);
  return Extract(JSVal::Bool(res == CMP_TRUE));
}

inline Rep BINARY_GTE(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const CompareResult res = JSVal::Compare<true>(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(res == CMP_FALSE));
}

inline Rep BINARY_INSTANCEOF(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  if (!rhs.IsObject()) {
    ctx->PendingError()->Report(Error::Type, "instanceof requires object");
    IV_LV5_BREAKER_RAISE();
  }
  JSObject* const robj = rhs.object();
  if (!robj->IsCallable()) {
    ctx->PendingError()->Report(Error::Type, "instanceof requires constructor");
    IV_LV5_BREAKER_RAISE();
  }
  const bool result = robj->AsCallable()->HasInstance(ctx, lhs, ERR);
  return Extract(JSVal::Bool(result));
}

inline Rep BINARY_IN(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  if (!rhs.IsObject()) {
    ctx->PendingError()->Report(Error::Type, "in requires object");
    IV_LV5_BREAKER_RAISE();
  }
  const Symbol s = lhs.ToSymbol(ctx, ERR);
  return Extract(JSVal::Bool(rhs.object()->HasProperty(ctx, s)));
}

inline Rep BINARY_EQ(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const bool result = JSVal::AbstractEqual(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(result));
}

inline Rep BINARY_STRICT_EQ(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  return Extract(JSVal::Bool(JSVal::StrictEqual(lhs, rhs)));
}

inline Rep BINARY_NE(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const bool result = JSVal::AbstractEqual(ctx, lhs, rhs, ERR);
  return Extract(JSVal::Bool(!result));
}

inline Rep BINARY_STRICT_NE(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  return Extract(JSVal::Bool(!JSVal::StrictEqual(lhs, rhs)));
}

inline Rep BINARY_BIT_AND(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left & right));
}

inline Rep BINARY_BIT_XOR(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left ^ right));
}

inline Rep BINARY_BIT_OR(railgun::Context* ctx, JSVal lhs, JSVal rhs) {
  const int32_t left = lhs.ToInt32(ctx, ERR);
  const int32_t right = rhs.ToInt32(ctx, ERR);
  return Extract(JSVal::Int32(left | right));
}

inline Rep TO_NUMBER(railgun::Context* ctx, JSVal src) {
  const double x = src.ToNumber(ctx, ERR);
  return Extract(x);
}

inline Rep UNARY_NEGATIVE(railgun::Context* ctx, JSVal src) {
  const double x = src.ToNumber(ctx, ERR);
  return Extract(-x);
}

inline JSVal UNARY_NOT(JSVal src) {
  return JSVal::Bool(!src.ToBoolean());
}

inline Rep UNARY_BIT_NOT(railgun::Context* ctx, JSVal src) {
  const double value = src.ToNumber(ctx, ERR);
  return Extract(JSVal::Int32(~core::DoubleToInt32(value)));
}

inline Rep THROW(railgun::Context* ctx, JSVal src) {
  ctx->PendingError()->Report(src);
  IV_LV5_BREAKER_RAISE();
}

inline void POP_ENV(railgun::Frame* frame) {
  frame->set_lexical_env(frame->lexical_env()->outer());
}

inline void RaiseReferenceError(Symbol name, Error* e) {
  core::UStringBuilder builder;
  builder.Append('"');
  builder.Append(symbol::GetSymbolString(name));
  builder.Append("\" not defined");
  e->Report(Error::Reference, builder.BuildPiece());
}

inline JSEnv* GetEnv(railgun::Context* ctx, JSEnv* env, Symbol name) {
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
inline Rep LOAD_GLOBAL(railgun::Context* ctx,
                       Symbol name, railgun::Instruction* instr) {
  // opcode | (dst | index) | nop | nop
  JSGlobal* global = ctx->global_obj();
  if (instr[2].map == global->map()) {
    // map is cached, so use previous index code
    const JSVal res = global->GetBySlotOffset(ctx, instr[3].u32[0], ERR);
    return Extract(res);
  } else {
    Slot slot;
    if (global->GetOwnPropertySlot(ctx, name, &slot)) {
      // now Own Property Pattern only implemented
      assert(slot.IsCacheable());
      instr[2].map = global->map();
      instr[3].u32[0] = slot.offset();
      const JSVal res = slot.Get(ctx, global, ERR);
      return Extract(res);
    } else {
      instr[2].map = NULL;
      if (JSEnv* current = GetEnv(ctx, ctx->global_env(), name)) {
        const JSVal res = current->GetBindingValue(ctx, name, STRICT, ERR);
        return Extract(res);
      }
      RaiseReferenceError(name, ERR);
      return 0;
    }
  }
}

template<bool STRICT>
inline Rep STORE_GLOBAL(railgun::Context* ctx,
                        JSVal src, Symbol name, railgun::Instruction* instr) {
  // opcode | (src | name) | nop | nop
  JSGlobal* global = ctx->global_obj();
  if (instr[2].map == global->map()) {
    // map is cached, so use previous index code
    global->PutToSlotOffset(ctx, instr[3].u32[0], src, STRICT, ERR);
  } else {
    Slot slot;
    if (global->GetOwnPropertySlot(ctx, name, &slot)) {
      instr[2].map = global->map();
      instr[3].u32[0] = slot.offset();
      global->PutToSlotOffset(ctx, instr[3].u32[0], src, STRICT, ERR);
    } else {
      instr[2].map = NULL;
      if (JSEnv* current = GetEnv(ctx, ctx->global_env(), name)) {
        current->SetMutableBinding(ctx, name, src, STRICT, ERR);
      } else {
        if (STRICT) {
          ctx->PendingError()->Report(Error::Reference,
                    "putting to unresolvable reference "
                    "not allowed in strict reference");
          IV_LV5_BREAKER_RAISE();
        } else {
          ctx->global_obj()->Put(ctx, name, src, STRICT, ERR);
        }
      }
    }
  }
  return 0;
}

inline Rep DELETE_GLOBAL(railgun::Context* ctx, Symbol name) {
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
inline Rep TYPEOF_GLOBAL(railgun::Context* ctx, Symbol name) {
  JSEnv* global = ctx->global_env();
  if (global->HasBinding(ctx, name)) {
    const JSVal res = global->GetBindingValue(ctx, name, STRICT, ERR);
    return Extract(res.TypeOf(ctx));
  } else {
    // not found -> unresolvable reference
    return Extract(ctx->global_data()->string_undefined());
  }
}

inline JSDeclEnv* GetHeapEnv(JSEnv* env, uint32_t nest) {
  for (uint32_t i = 0; i < nest; ++i) {
    env = env->outer();
  }
  assert(env->AsJSDeclEnv());
  return static_cast<JSDeclEnv*>(env);
}

template<bool STRICT>
inline Rep LOAD_HEAP(railgun::Context* ctx, JSEnv* env,
                     uint32_t offset, uint32_t nest) {
  const JSVal res = GetHeapEnv(env, nest)->GetByOffset(offset, STRICT, ERR);
  return Extract(res);
}

template<bool STRICT>
inline Rep STORE_HEAP(railgun::Context* ctx, JSEnv* env,
                      uint32_t offset, uint32_t nest, JSVal src) {
  GetHeapEnv(env, nest)->SetByOffset(offset, src, STRICT, ERR);
  return 0;
}

template<int Target, std::size_t Returned, bool STRICT>
JSVal IncrementHeap(railgun::Context* ctx, JSEnv* env,
                    uint32_t offset, uint32_t nest) {
  JSDeclEnv* decl = GetHeapEnv(env, nest);
  const JSVal w = decl->GetByOffset(offset, STRICT, ERR);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    decl->SetByOffset(offset, std::get<1>(results), STRICT, ERR);
    return std::get<Returned>(results);
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, ERR);
    std::get<1>(results) = std::get<0>(results) + Target;
    decl->SetByOffset(offset, std::get<1>(results), STRICT, ERR);
    return std::get<Returned>(results);
  }
}

template<bool STRICT>
inline Rep TYPEOF_HEAP(railgun::Context* ctx, JSEnv* env,
                       uint32_t offset, uint32_t nest) {
  const JSVal res = GetHeapEnv(env, nest)->GetByOffset(offset, STRICT, ERR);
  return Extract(res.TypeOf(ctx));
}

template<int Target, std::size_t Returned, bool STRICT>
JSVal IncrementName(railgun::Context* ctx, JSEnv* env, Symbol s, Error* e) {
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
JSVal IncrementGlobalWithSlot(Context* ctx,
                              JSGlobal* global,
                              std::size_t slot, Error* e) {
  const JSVal w = global->GetBySlotOffset(ctx, slot, e);
  if (w.IsInt32() &&
      railgun::detail::IsIncrementOverflowSafe<Target>(w.int32())) {
    std::tuple<JSVal, JSVal> results;
    const int32_t target = w.int32();
    std::get<0>(results) = w;
    std::get<1>(results) = JSVal::Int32(target + Target);
    global->PutToSlotOffset(ctx, slot, std::get<1>(results), STRICT, e);
    return std::get<Returned>(results);
  } else {
    std::tuple<double, double> results;
    std::get<0>(results) = w.ToNumber(ctx, IV_LV5_ERROR(e));
    std::get<1>(results) = std::get<0>(results) + Target;
    global->PutToSlotOffset(ctx, slot, std::get<1>(results), STRICT, e);
    return std::get<Returned>(results);
  }
}

template<int Target, std::size_t Returned, bool STRICT>
JSVal IncrementGlobal(railgun::Context* ctx,
                      railgun::Instruction* instr, Symbol s) {
  // opcode | (dst | name) | nop | nop
  JSGlobal* global = ctx->global_obj();
  if (instr[2].map == global->map()) {
    // map is cached, so use previous index code
    const JSVal res =
        IncrementGlobalWithSlot<Target,
                                Returned,
                                STRICT>(ctx, global, instr[3].u32[0], ERR);
    return res;
  } else {
    Slot slot;
    if (global->GetOwnPropertySlot(ctx, s, &slot)) {
      instr[2].map = global->map();
      instr[3].u32[0] = slot.offset();
      const JSVal res =
          IncrementGlobalWithSlot<Target,
                                  Returned,
                                  STRICT>(ctx, global, instr[3].u32[0], ERR);
      return res;
    } else {
      instr[2].map = NULL;
      const JSVal res =
          IncrementName<Target,
                        Returned,
                        STRICT>(ctx, ctx->global_env(), s, ERR);
      return res;
    }
  }
}

inline Rep CALL(railgun::Context* ctx,
                JSVal callee,
                JSVal* offset,
                uint64_t argc_with_this,
                railgun::Frame** out_frame) {
  if (!callee.IsCallable()) {
    ctx->PendingError()->Report(Error::Type, "not callable object");
    IV_LV5_BREAKER_RAISE();
  }
  JSFunction* func = callee.object()->AsCallable();
  if (!func->IsNativeFunction()) {
    // inline call
    railgun::JSVMFunction* vm_func = static_cast<railgun::JSVMFunction*>(func);
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
        NULL,  // TODO(Constellation) set precise position
        argc_with_this, false);
    if (!new_frame) {
      ctx->PendingError()->Report(Error::Range,
                                  "maximum call stack size exceeded");
      IV_LV5_BREAKER_RAISE();
    }
    new_frame->InitThisBinding(ctx, ERR);
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

inline Rep EVAL(railgun::Context* ctx,
                JSVal callee,
                JSVal* offset,
                uint64_t argc_with_this,
                railgun::Frame** out_frame) {
  if (!callee.IsCallable()) {
    ctx->PendingError()->Report(Error::Type, "not callable object");
    IV_LV5_BREAKER_RAISE();
  }
  JSFunction* func = callee.object()->AsCallable();
  if (!func->IsNativeFunction()) {
    // inline call
    railgun::JSVMFunction* vm_func = static_cast<railgun::JSVMFunction*>(func);
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
        NULL,  // TODO(Constellation) set precise position
        argc_with_this, false);
    if (!new_frame) {
      ctx->PendingError()->Report(Error::Range,
                                  "maximum call stack size exceeded");
      IV_LV5_BREAKER_RAISE();
    }
    new_frame->InitThisBinding(ctx, ERR);
    *out_frame = new_frame;
    return reinterpret_cast<Rep>(code->executable());
  }

  // Native Function
  {
    railgun::detail::VMArguments args(ctx,
                                      offset + (argc_with_this - 1),
                                      argc_with_this - 1);
    const JSAPI native = func->NativeFunction();
    if (native && native == &railgun::GlobalEval) {
      // direct call to eval point
      args.set_this_binding(args.this_binding());
      const JSVal res = DirectCallToEval(args, *out_frame, ERR);
      return Extract(res);
    }
    const JSVal res = func->Call(&args, args.this_binding(), ERR);
    return Extract(res);
  }
}

inline Rep CONSTRUCT(railgun::Context* ctx,
                     JSVal callee,
                     JSVal* offset,
                     uint64_t argc_with_this,
                     railgun::Frame** out_frame) {
  if (!callee.IsCallable()) {
    ctx->PendingError()->Report(Error::Type, "not callable object");
    IV_LV5_BREAKER_RAISE();
  }
  JSFunction* func = callee.object()->AsCallable();
  if (!func->IsNativeFunction()) {
    // inline call
    railgun::JSVMFunction* vm_func = static_cast<railgun::JSVMFunction*>(func);
    railgun::Code* code = vm_func->code();
    railgun::Frame* new_frame = ctx->vm()->stack()->NewCodeFrame(
        ctx,
        offset,
        code,
        vm_func->scope(),
        func,
        NULL,  // TODO(Constellation) set precise position
        argc_with_this, false);
    if (!new_frame) {
      ctx->PendingError()->Report(Error::Range,
                                  "maximum call stack size exceeded");
      IV_LV5_BREAKER_RAISE();
    }
    JSObject* const obj = JSObject::New(ctx, code->ConstructMap(ctx));
    new_frame->set_this_binding(obj);
    const JSVal proto = func->Get(ctx, symbol::prototype(), ERR);
    if (proto.IsObject()) {
      obj->set_prototype(proto.object());
    }
    new_frame->InitThisBinding(ctx, ERR);
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

inline Rep CONCAT(railgun::Context* ctx, JSVal* src, uint32_t count) {
  return Extract(JSString::New(ctx, src, count));
}

inline Rep RAISE_REFERENCE(railgun::Context* ctx, Symbol name) {
  core::UStringBuilder builder;
  builder.Append("Invalid left-hand side expression");
  ctx->PendingError()->Report(Error::Reference, builder.BuildPiece());
  IV_LV5_BREAKER_RAISE();
  return 0;
}

inline Rep RAISE_IMMUTABLE(railgun::Context* ctx, Symbol name) {
  core::UStringBuilder builder;
  builder.Append("mutating immutable binding \"");
  builder.Append(symbol::GetSymbolString(name));
  builder.Append("\" not allowed in strict mode");
  ctx->PendingError()->Report(Error::Type, builder.BuildPiece());
  IV_LV5_BREAKER_RAISE();
  return 0;
}

inline Rep TYPEOF(railgun::Context* ctx, JSVal src) {
  return Extract(src.TypeOf(ctx));
}

inline Rep TO_PRIMITIVE_AND_TO_STRING(railgun::Context* ctx, JSVal src) {
  const JSVal primitive = src.ToPrimitive(ctx, Hint::NONE, ERR);
  JSString* str = primitive.ToString(ctx, ERR);
  return Extract(str);
}

template<bool MERGED>
inline void STORE_OBJECT_DATA(railgun::Context* ctx,
                              JSVal target, JSVal item, uint32_t offset) {
  JSObject* obj = target.object();
  if (MERGED) {
    obj->GetSlot(offset) =
        PropertyDescriptor::Merge(
            DataDescriptor(item, ATTR::W | ATTR::E | ATTR::C),
        obj->GetSlot(offset));
  } else {
    obj->GetSlot(offset) =
        DataDescriptor(item, ATTR::W | ATTR::E | ATTR::C);
  }
}

template<bool MERGED>
inline void STORE_OBJECT_GET(railgun::Context* ctx,
                             JSVal target, JSVal item, uint32_t offset) {
  JSObject* obj = target.object();
  if (MERGED) {
    obj->GetSlot(offset) =
        PropertyDescriptor::Merge(
            AccessorDescriptor(item.object(), NULL,
                               ATTR::E | ATTR::C | ATTR::UNDEF_SETTER),
        obj->GetSlot(offset));
  } else {
    obj->GetSlot(offset) =
        AccessorDescriptor(item.object(), NULL,
                           ATTR::E | ATTR::C | ATTR::UNDEF_SETTER);
  }
}

template<bool MERGED>
inline void STORE_OBJECT_SET(railgun::Context* ctx,
                             JSVal target, JSVal item, uint32_t offset) {
  JSObject* obj = target.object();
  if (MERGED) {
    obj->GetSlot(offset) =
        PropertyDescriptor::Merge(
            AccessorDescriptor(NULL, item.object(),
                               ATTR::E | ATTR::C | ATTR::UNDEF_GETTER),
        obj->GetSlot(offset));
  } else {
    obj->GetSlot(offset) =
        AccessorDescriptor(NULL, item.object(),
                           ATTR::E | ATTR::C | ATTR::UNDEF_GETTER);
  }
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
inline Rep INSTANTIATE_DECLARATION_BINDING(railgun::Context* ctx,
                                           JSEnv* env, Symbol name) {
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
      if (existing_prop.IsAccessorDescriptor()) {
        ctx->PendingError()->Report(Error::Type,
                                    "create mutable function binding failed");
        IV_LV5_BREAKER_RAISE();
      }
      const DataDescriptor* const data = existing_prop.AsDataDescriptor();
      if (!data->IsWritable() || !data->IsEnumerable()) {
        ctx->PendingError()->Report(Error::Type,
                                    "create mutable function binding failed");
        IV_LV5_BREAKER_RAISE();
      }
    }
  }
  return 0;
}

template<bool CONFIGURABLE, bool STRICT>
inline Rep INSTANTIATE_VARIABLE_BINDING(railgun::Context* ctx,
                                        JSEnv* env, Symbol name) {
  // opcode | (name | configurable)
  if (!env->HasBinding(ctx, name)) {
    env->CreateMutableBinding(ctx, name, CONFIGURABLE, ERR);
    env->SetMutableBinding(ctx, name, JSUndefined, STRICT, ERR);
  }
  return 0;
}

inline void INITIALIZE_HEAP_IMMUTABLE(JSEnv* env, JSVal src, uint32_t offset) {
  static_cast<JSDeclEnv*>(env)->InitializeImmutable(offset, src);
}

inline Rep INCREMENT(railgun::Context* ctx, JSVal src) {
  const double res = src.ToNumber(ctx, ERR);
  return Extract(res + 1);
}


inline Rep DECREMENT(railgun::Context* ctx, JSVal src) {
  const double res = src.ToNumber(ctx, ERR);
  return Extract(res - 1);
}

inline Rep POSTFIX_INCREMENT(railgun::Context* ctx,
                             JSVal val, JSVal* src) {
  const double res = val.ToNumber(ctx, ERR);
  *src = res;
  return Extract(res + 1);
}

inline Rep POSTFIX_DECREMENT(railgun::Context* ctx,
                             JSVal val, JSVal* src) {
  const double res = val.ToNumber(ctx, ERR);
  *src = res;
  return Extract(res - 1);
}

inline bool TO_BOOLEAN(JSVal src) {
  return src.ToBoolean();
}

template<bool STRICT>
inline Rep LOAD_ARGUMENTS(railgun::Context* ctx, railgun::Frame* frame) {
  if (STRICT) {
    JSObject* obj = JSStrictArguments::New(
        ctx, frame->callee().object()->AsCallable(),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        ERR);
    return Extract(obj);
  } else {
    JSObject* obj = JSNormalArguments::New(
        ctx, frame->callee().object()->AsCallable(),
        frame->code()->params(),
        frame->arguments_crbegin(),
        frame->arguments_crend(),
        static_cast<JSDeclEnv*>(frame->variable_env()),
        ERR);
    return Extract(obj);
  }
}

inline Rep PREPARE_DYNAMIC_CALL(railgun::Context* ctx,
                                JSEnv* env,
                                Symbol name,
                                JSVal* base) {
  if (JSEnv* target_env = GetEnv(ctx, env, name)) {
    const JSVal res = target_env->GetBindingValue(ctx, name, false, ERR);
    *base = res;
    return Extract(target_env->ImplicitThisValue());
  }
  RaiseReferenceError(name, ctx->PendingError());
  IV_LV5_BREAKER_RAISE();
}

template<bool STRICT>
inline Rep LOAD_NAME(railgun::Context* ctx, JSEnv* env, Symbol name) {
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    const JSVal res = current->GetBindingValue(ctx, name, STRICT, ERR);
    return Extract(res);
  }
  RaiseReferenceError(name, ctx->PendingError());
  IV_LV5_BREAKER_RAISE();
}

template<bool STRICT>
inline Rep STORE_NAME(railgun::Context* ctx, JSEnv* env, Symbol name, JSVal src) {
  if (JSEnv* current = GetEnv(ctx, env, name)) {
    current->SetMutableBinding(ctx, name, src, STRICT, ERR);
  } else {
    if (STRICT) {
      ctx->PendingError()->Report(
          Error::Reference,
          "putting to unresolvable reference "
          "not allowed in strict reference");
      IV_LV5_BREAKER_RAISE();
    } else {
      ctx->global_obj()->Put(ctx, name, src, STRICT, ERR);
    }
  }
  return 0;
}

#undef ERR
} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_STUB_H_
