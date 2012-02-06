#ifndef IV_LV5_NAME_ITERATOR_H_
#define IV_LV5_NAME_ITERATOR_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {

class NameIterator : public radio::HeapObject<> {
 public:
  NameIterator(Context* ctx, JSObject* obj)
    : keys_(),
      iter_() {
    PropertyNamesCollector collector;
    obj->GetPropertyNames(ctx, &collector, JSObject::EXCLUDE_NOT_ENUMERABLE);
    keys_.assign(collector.names().begin(), collector.names().end());
    iter_ = keys_.begin();
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

  static NameIterator* New(Context* ctx, JSObject* obj) {
    return new NameIterator(ctx, obj);
  }

  void MarkChildren(radio::Core* core) { }

 private:
  GCVector<Symbol>::type keys_;
  GCVector<Symbol>::type::const_iterator iter_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_NAME_ITERATOR_H_
