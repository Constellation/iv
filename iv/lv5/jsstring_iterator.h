#ifndef IV_LV5_JSSTRING_ITERATOR_H_
#define IV_LV5_JSSTRING_ITERATOR_H_
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

typedef JSObjectWithTuple<JSString*, uint32_t> JSStringIteratorSuper;

class JSStringIterator : public JSStringIteratorSuper {
 public:
  enum Index {
    ITERATED = 0,
    INDEX = 1
  };

  static JSStringIterator* New(Context* ctx, JSString* str) {
    return new JSStringIterator(
        ctx,
        ctx->global_data()->string_iterator_map(),
        Unique(),
        str,
        0U);
  }

  static JSStringIterator* As(JSObject* obj) {
    if (obj->IsTuple() &&
        static_cast<JSObjectTuple*>(obj)->unique() == Unique()) {
      return static_cast<JSStringIterator*>(obj);
    }
    return nullptr;
  }

 private:
  static uintptr_t Unique() {
    static const char* const kUniqueSymbol = "StringIteratorKey";
    return reinterpret_cast<uintptr_t>(kUniqueSymbol);
  }

  IV_FORWARD_CONSTRUCTOR(JSStringIterator, JSStringIteratorSuper)
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_ITERATOR_H_
