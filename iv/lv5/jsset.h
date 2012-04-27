// This is ES.next Set implementation
//
// detail
// http://wiki.ecmascript.org/doku.php?id=harmony:simple_maps_and_sets
//
#ifndef IV_LV5_JSSET_H_
#define IV_LV5_JSSET_H_
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class Context;

class JSSet : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(Set)

  typedef GCHashSet<JSVal, JSVal::Hasher, JSVal::SameValueEqualer>::type Set;

  JSSet(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      set_() {
  }

  JSSet(Context* ctx, Map* map)
    : JSObject(map),
      set_() {
  }

  static JSSet* New(Context* ctx) {
    JSSet* const set = new JSSet(ctx);
    set->set_cls(JSSet::GetClass());
    set->set_prototype(context::GetClassSlot(ctx, Class::Set).prototype);
    return set;
  }

  static JSSet* NewPlain(Context* ctx, Map* map) {
    return new JSSet(ctx, map);
  }

  void AddKey(JSVal key) {
    set_.insert(key);
  }

  bool HasKey(JSVal key) const {
    return set_.find(key) != set_.end();
  }

  bool DeleteKey(JSVal key) {
    Set::iterator it = set_.find(key);
    if (it != set_.end()) {
      set_.erase(it);
      return true;
    }
    return false;
  }

 private:
  Set set_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSET_H_
