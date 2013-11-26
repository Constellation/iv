#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsmap.h>
#include <iv/lv5/jsmap_iterator.h>
#include <iv/lv5/runtime/map.h>
namespace iv {
namespace lv5 {
namespace runtime {

// 15.14.2.1 Map(iterable = [])
// 15.14.3.1 new Map (iterable = [])
JSVal MapConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  const JSVal first = args.At(0);
  const JSVal this_binding = args.this_binding();
  JSObject* map = nullptr;
  if (this_binding.IsUndefined() ||
      JSVal::StrictEqual(this_binding, ctx->global_data()->map_prototype())) {
    map = JSObject::New(
        ctx,
        ctx->global_data()->map_map());
  } else {
    map = this_binding.ToObject(ctx, IV_LV5_ERROR(e));
  }
  return JSMap::Initialize(ctx, map, first, IV_LV5_ERROR(e));
}

// 23.1.3.1 Map.prototype.clear()
JSVal MapClear(const Arguments& args, Error* e) {
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

// 23.1.3.3 Map.prototype.delete(key)
JSVal MapDelete(const Arguments& args, Error* e) {
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

// 23.1.3.4 Map.prototype.entries()
JSVal MapEntries(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.entries", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.entries is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return JSMapIterator::New(ctx, entries, MapIterationKind::KEY_PLUS_VALUE);
}

// 23.1.3.5 Map.prototype.forEach(callbackfn, thisArg = undefined)
JSVal MapForEach(const Arguments& args, Error* e) {
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

// 23.1.3.6 Map.prototype.get(key)
JSVal MapGet(const Arguments& args, Error* e) {
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

// 23.1.3.7 Map.prototype.has(key)
JSVal MapHas(const Arguments& args, Error* e) {
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

// 23.1.3.8 Map.prototype.keys()
JSVal MapKeys(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.keys", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.keysis not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return JSMapIterator::New(ctx, entries, MapIterationKind::KEY);
}

// 23.1.3.9 Map.prototype.set(key, value)
JSVal MapSet(const Arguments& args, Error* e) {
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

// 23.1.3.10 get Map.prototype.size
JSVal MapSize(const Arguments& args, Error* e) {
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

// 23.1.3.11 Map.prototype.values()
JSVal MapValues(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.values", args, e);
  Context* ctx = args.ctx();
  JSObject* map = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  Slot slot;
  if (!map->GetOwnPropertySlot(ctx, JSMap::symbol(), &slot)) {
    e->Report(Error::Type, "Map.prototype.values is not generic function");
    return JSEmpty;
  }
  JSMap::Data* entries = static_cast<JSMap::Data*>(slot.value().cell());
  return JSMapIterator::New(ctx, entries, MapIterationKind::VALUE);
}

} } }  // namespace iv::lv5::runtime
