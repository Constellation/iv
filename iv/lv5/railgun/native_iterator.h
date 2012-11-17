#ifndef IV_LV5_RAILGUN_NAME_ITERATOR_H_
#define IV_LV5_RAILGUN_NAME_ITERATOR_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

// NativeIterator is not GC managed object
class NativeIterator
  : public radio::HeapObject<radio::NATIVE_ITERATOR>,
    public PropertyNamesCollector {
 public:
  NativeIterator()
    : PropertyNamesCollector(),
      iter_() {
  }

  Symbol Get() const {
    return *iter_;
  }

  bool Has() const {
    return iter_ != names().end();
  }

  void Next() {
    ++iter_;
  }

  void Fill(Context* ctx, JSObject* obj) {
    Clear();
    obj->GetPropertyNames(ctx, this, EXCLUDE_NOT_ENUMERABLE);
    iter_ = names().begin();
  }

  // string fast path
  void Fill(Context* ctx, JSString* str) {
    Clear();
    for (uint32_t i = 0, len = str->size(); i < len; ++i) {
      Add(i);
    }
    JSObject* proto = ctx->global_data()->string_prototype();
    proto->GetPropertyNames(ctx, LevelUp(), EXCLUDE_NOT_ENUMERABLE);
    iter_ = names().begin();
  }
 private:
  PropertyNamesCollector::Names::const_iterator iter_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_NAME_ITERATOR_H_
