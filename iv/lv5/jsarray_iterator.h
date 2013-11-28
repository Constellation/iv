#ifndef IV_LV5_JSARRAY_ITERATOR_H_
#define IV_LV5_JSARRAY_ITERATOR_H_
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
namespace iv {
namespace lv5 {

enum class ArrayIterationKind {
  KEY,
  VALUE,
  KEY_PLUS_VALUE,
  SPARSE_KEY,
  SPARSE_VALUE,
  SPARSE_KEY_PLUS_VALUE
};
typedef JSObjectWithTuple<
              JSObject*, uint32_t, ArrayIterationKind> JSArrayIteratorSuper;

class JSArrayIterator : public JSArrayIteratorSuper {
 public:
  enum Index {
    ITERATED = 0,
    INDEX = 1,
    KIND = 2
  };

  static JSArrayIterator* New(Context* ctx,
                              JSObject* iterated, ArrayIterationKind kind) {
    return new JSArrayIterator(
        ctx,
        ctx->global_data()->array_iterator_map(),
        Unique(),
        iterated,
        0U,
        kind);
  }

  static JSArrayIterator* As(JSObject* obj) {
    if (obj->IsTuple() &&
        static_cast<JSObjectTuple*>(obj)->unique() == Unique()) {
      return static_cast<JSArrayIterator*>(obj);
    }
    return nullptr;
  }

 private:
  static uintptr_t Unique() {
    static const char* const kUniqueSymbol = "ArrayIteratorKey";
    return reinterpret_cast<uintptr_t>(kUniqueSymbol);
  }

  IV_FORWARD_CONSTRUCTOR(JSArrayIterator, JSArrayIteratorSuper)
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSARRAY_ITERATOR_H_
