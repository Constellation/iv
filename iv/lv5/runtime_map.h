#ifndef IV_LV5_RUNTIME_MAP_H_
#define IV_LV5_RUNTIME_MAP_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsmap.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.14.2.1 Map(iterable = [])
// section 15.14.3.1 new Map (iterable = [])
inline JSVal MapConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  const JSVal first = args.At(0);
  const JSVal this_binding = args.this_binding();
  JSObject* map = NULL;
  if (this_binding.IsUndefined() ||
      JSVal::StrictEqual(this_binding, ctx->global_data()->map_prototype())) {
    map = JSObject::New(
        ctx,
        ctx->global_data()->map_map(),
        ctx->global_data()->map_prototype());
  } else {
    map = this_binding.ToObject(ctx, IV_LV5_ERROR(e));
  }
  return JSMap::Initialize(ctx, map, first, IV_LV5_ERROR(e));
}

// section 15.14.5.2 Map.prototype.clear()
inline JSVal MapClear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.clear", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.clear is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  entries->Clear();
  return JSUndefined;
}

// section 15.14.5.3 Map.prototype.delete(key)
inline JSVal MapDelete(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.delete", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.delete is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return JSVal::Bool(entries->Delete(args.At(0)));
}

// 15.14.5.4 Map.prototype.forEach(callbackfn, thisArg = undefined)
inline JSVal MapForEach(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.forEach", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.forEach is not generic function");
    return JSEmpty;
  }
  const JSVal callbackfn = args.At(0);
  if (!callbackfn.IsCallable()) {
    e->Report(Error::Type, "Map.prototype.forEach callbackfn is not callable");
    return JSEmpty;
  }
  JSFunction* callback = static_cast<JSFunction*>(callbackfn.object());
  JSVal this_arg = args.At(1);
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  for (JSMap::Data::Mapping::const_iterator it = entries->mapping().begin(),
       last = entries->mapping().end(); it != last; ++it) {
    ScopedArguments arg_list(ctx, 3, IV_LV5_ERROR(e));
    arg_list[0] = it->first;
    arg_list[1] = it->second;
    arg_list[2] = map;
    callback->Call(&arg_list, this_arg, IV_LV5_ERROR(e));
  }
  return JSUndefined;
}

// section 15.14.5.5 Map.prototype.get(key)
inline JSVal MapGet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.get", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.get is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return entries->Get(args.At(0));
}

// section 15.14.5.6 Map.prototype.has(key)
inline JSVal MapHas(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.has", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.has is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return JSVal::Bool(entries->Has(args.At(0)));
}

// TODO(Constellation) iv / lv5 doesn't have iterator system
// 15.14.5.7 Map.prototype.items()
// 15.14.5.8 Map.prototype.keys()

// 15.14.5.9 Map.prototype.set(key, value)
inline JSVal MapSet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.set", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.set is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  entries->Set(args.At(0), args.At(1));
  return JSUndefined;
}

// 15.14.5.9 get Map.prototype.size
inline JSVal MapSize(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.size", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.size is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return JSVal::UInt32(static_cast<uint32_t>(entries->mapping().size()));
}

// TODO(Constellation) iv / lv5 doesn't have iterator system
// 15.14.5.10 Map.prototype.values()
// 15.14.5.11 Map.prototype.@@iterator

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MAP_H_
