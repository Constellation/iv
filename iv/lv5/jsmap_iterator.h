#ifndef IV_LV5_JSMAP_ITERATOR_H_
#define IV_LV5_JSMAP_ITERATOR_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsobject_with_tuple.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/class.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsmap.h>
namespace iv {
namespace lv5 {

enum class MapIterationKind {
  KEY,
  VALUE,
  KEY_PLUS_VALUE
};

typedef JSObjectWithTuple<
  JSMap::Data*, uint32_t, MapIterationKind> JSMapIteratorSuper;

class JSMapIterator : public JSMapIteratorSuper {
 public:
  enum Index {
    MAP = 0,
    INDEX = 1,
    KIND = 2
  };

  static JSMapIterator* New(Context* ctx,
                            JSMap::Data* map, MapIterationKind kind) {
    return new JSMapIterator(
        ctx,
        ctx->global_data()->map_iterator_map(),
        Unique(),
        map,
        0U,
        kind);
  }

  static JSMapIterator* As(JSObject* obj) {
    if (obj->IsTuple() &&
        static_cast<JSObjectTuple*>(obj)->unique() == Unique()) {
      return static_cast<JSMapIterator*>(obj);
    }
    return nullptr;
  }

 private:
  static uintptr_t Unique() {
    static const char* const kUniqueSymbol = "MapIteratorKey";
    return reinterpret_cast<uintptr_t>(kUniqueSymbol);
  }

  using JSMapIteratorSuper::JSMapIteratorSuper;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSMAP_ITERATOR_H_
