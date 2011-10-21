#ifndef IV_LV5_NAME_ITERATOR_H_
#define IV_LV5_NAME_ITERATOR_H_
#include <gc/gc_cpp.h>
#include "lv5/jsobject.h"
#include "lv5/radio/cell.h"
namespace iv {
namespace lv5 {

class NameIterator : public radio::HeapObject<> {
 public:
  NameIterator(Context* ctx, JSObject* obj)
    : keys_(),
      iter_() {
    std::vector<Symbol> keys;
    obj->GetPropertyNames(ctx, &keys, JSObject::EXCLUDE_NOT_ENUMERABLE);
    keys_.assign(keys.begin(), keys.end());
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

 private:
  GCVector<Symbol>::type keys_;
  GCVector<Symbol>::type::const_iterator iter_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_NAME_ITERATOR_H_
