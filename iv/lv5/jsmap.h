// This is ES.next Map implementation
//
// detail
// http://wiki.ecmascript.org/doku.php?id=harmony:simple_maps_and_sets
//
#ifndef IV_LV5_JSMAP_H_
#define IV_LV5_JSMAP_H_
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class Context;

class JSMap : public JSObject {
 public:
  typedef GCHashMap<JSVal, JSVal,
                    JSVal::Hasher, JSVal::SameValueEqualer>::type Mapping;

  JSMap(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      mapping_() {
  }

  JSMap(Context* ctx, Map* map)
    : JSObject(map),
      mapping_() {
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Map",
      Class::Map
    };
    return &cls;
  }

  static JSMap* New(Context* ctx) {
    JSMap* const map = new JSMap(ctx);
    map->set_cls(JSMap::GetClass());
    map->set_prototype(context::GetClassSlot(ctx, Class::Map).prototype);
    return map;
  }

  static JSMap* NewPlain(Context* ctx, Map* map) {
    return new JSMap(ctx, map);
  }

  JSVal GetValue(JSVal key) const {
    Mapping::const_iterator it = mapping_.find(key);
    if (it != mapping_.end()) {
      return it->second;
    }
    return JSUndefined;
  }

  void SetValue(JSVal key, JSVal val) {
    mapping_[key] = val;
  }

  bool HasKey(JSVal key) const {
    return mapping_.find(key) != mapping_.end();
  }

  bool DeleteKey(JSVal key) {
    Mapping::iterator it = mapping_.find(key);
    if (it != mapping_.end()) {
      mapping_.erase(it);
      return true;
    }
    return false;
  }

 private:
  Mapping mapping_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSMAP_H_
