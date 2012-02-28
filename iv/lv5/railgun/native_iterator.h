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
class NativeIterator : public radio::HeapObject<radio::NATIVE_ITERATOR> {
 public:
  NativeIterator()
    : keys_(),
      iter_() {
  }

  Symbol Get() const {
    return *iter_;
  }

  bool Has() const {
    return iter_ != keys_.end();
  }

  void Next() {
    ++iter_;
  }

  void Fill(Context* ctx, JSObject* obj) {
    keys_.clear();
    PropertyNamesCollector collector(&keys_);
    obj->GetPropertyNames(ctx, &collector, JSObject::EXCLUDE_NOT_ENUMERABLE);
    iter_ = keys_.begin();
  }

  // string fast path
  void Fill(Context* ctx, JSString* str) {
    keys_.clear();
    PropertyNamesCollector collector(&keys_);
    for (uint32_t i = 0, len = str->size(); i < len; ++i) {
      collector.Add(i);
    }
    JSObject* proto = context::GetClassSlot(ctx, Class::String).prototype;
    proto->GetPropertyNames(ctx, collector.LevelUp(),
                            JSObject::EXCLUDE_NOT_ENUMERABLE);
  }
 private:
  PropertyNamesCollector::Names keys_;
  PropertyNamesCollector::Names::const_iterator iter_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_NAME_ITERATOR_H_
