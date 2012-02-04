#ifndef IV_LV5_RUNTIME_MAP_H_
#define IV_LV5_RUNTIME_MAP_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsmap.h>

namespace iv {
namespace lv5 {
namespace runtime {

inline JSVal MapConstructor(const Arguments& args, Error* e) {
  return JSMap::New(args.ctx());
}

inline JSVal MapGet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.get", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Map>()) {
    JSMap* map = static_cast<JSMap*>(obj.object());
    return map->GetValue(args.At(0));
  }
  e->Report(Error::Type, "Map.prototype.get is not generic function");
  return JSUndefined;
}

inline JSVal MapHas(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.has", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Map>()) {
    JSMap* map = static_cast<JSMap*>(obj.object());
    return JSVal::Bool(map->HasKey(args.At(0)));
  }
  e->Report(Error::Type, "Map.prototype.has is not generic function");
  return JSUndefined;
}

inline JSVal MapSet(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.set", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Map>()) {
    JSMap* map = static_cast<JSMap*>(obj.object());
    map->SetValue(args.At(0), args.At(1));
    return JSUndefined;
  }
  e->Report(Error::Type, "Set.prototype.set is not generic function");
  return JSUndefined;
}

inline JSVal MapDelete(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Map.prototype.delete", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Map>()) {
    JSMap* map = static_cast<JSMap*>(obj.object());
    return JSVal::Bool(map->DeleteKey(args.At(0)));
  }
  e->Report(Error::Type, "Map.prototype.delete is not generic function");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_MAP_H_
