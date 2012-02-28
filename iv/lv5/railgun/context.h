#ifndef IV_LV5_RAILGUN_CONTEXT_H_
#define IV_LV5_RAILGUN_CONTEXT_H_
#include <iv/functor.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/native_iterator.h>
#include <iv/lv5/railgun/vm_fwd.h>
#include <iv/lv5/railgun/context_fwd.h>
#include <iv/lv5/railgun/runtime.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline Context::Context()
  : lv5::Context(),
    vm_(),
    direct_eval_map_(10),
    iterator_cache_() {
  vm_ = new(GC_MALLOC_UNCOLLECTABLE(sizeof(VM)))VM(this);
  Initialize<&FunctionConstructor, &GlobalEval>();
  RegisterStack(vm_->stack());
#ifdef DEBUG
  iterator_live_count_ = 0;
#endif
}

inline NativeIterator* Context::GainNativeIterator(JSObject* obj) {
  NativeIterator* iterator = GainNativeIterator();
  iterator->Fill(this, obj);
  return iterator;
}

inline NativeIterator* Context::GainNativeIterator(JSString* str) {
  NativeIterator* iterator = GainNativeIterator();
  iterator->Fill(this, str);
  return iterator;
}

inline NativeIterator* Context::GainNativeIterator() {
  if (!iterator_cache_.empty()) {
    NativeIterator* iterator = iterator_cache_.back();
    iterator_cache_.pop_back();
    return iterator;
  } else {
#ifdef DEBUG
    ++iterator_live_count_;
#endif
    return new NativeIterator();
  }
}

inline void Context::ReleaseNativeIterator(NativeIterator* iterator) {
#ifdef DEBUG
  assert(iterator_live_count_ > 0);
  --iterator_live_count_;
  delete iterator;
#else
  if (iterator_cache_.size() < kNativeIteratorCacheMax) {
    iterator_cache_.push_back(iterator);
  } else {
    delete iterator;
  }
#endif
}

inline void Context::Validate() {
  assert(iterator_live_count_ == 0);
}

inline Context::~Context() {
  vm_->~VM();
  GC_FREE(vm_);
  std::for_each(iterator_cache_.begin(),
                iterator_cache_.end(),
                core::Deleter<NativeIterator>());
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONTEXT_H_
