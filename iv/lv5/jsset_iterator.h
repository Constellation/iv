#ifndef IV_LV5_JSSET_ITERATOR_H_
#define IV_LV5_JSSET_ITERATOR_H_
#include <iv/forward.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsobject_with_tuple.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/class.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsset.h>
namespace iv {
namespace lv5 {

enum class SetIterationKind {
  KEY,
  VALUE,
  KEY_PLUS_VALUE
};

typedef JSObjectWithTuple<
  JSSet::Data*, uint32_t, SetIterationKind> JSSetIteratorSuper;

class JSSetIterator : public JSSetIteratorSuper {
 public:
  enum Index {
    SET = 0,
    INDEX = 1,
    KIND = 2
  };

  static JSSetIterator* New(Context* ctx,
                            JSSet::Data* set, SetIterationKind kind) {
    return new JSSetIterator(
        ctx,
        ctx->global_data()->set_iterator_map(),
        Unique(),
        set,
        0U,
        kind);
  }

  static JSSetIterator* As(JSObject* obj) {
    if (obj->IsTuple() &&
        static_cast<JSObjectTuple*>(obj)->unique() == Unique()) {
      return static_cast<JSSetIterator*>(obj);
    }
    return nullptr;
  }

 private:
  static uintptr_t Unique() {
    static const char* const kUniqueSymbol = "SetIteratorKey";
    return reinterpret_cast<uintptr_t>(kUniqueSymbol);
  }

  IV_FORWARD_CONSTRUCTOR(JSSetIterator, JSSetIteratorSuper)
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSET_ITERATOR_H_
