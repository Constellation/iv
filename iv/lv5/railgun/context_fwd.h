#ifndef IV_LV5_RAILGUN_CONTEXT_FWD_H_
#define IV_LV5_RAILGUN_CONTEXT_FWD_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/lru_code_map.h>
#include <iv/lv5/factory.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Context : public lv5::Context {
 public:
  static const std::size_t kNativeIteratorCacheMax = 20;

  explicit Context();
  ~Context();

  VM* vm() {
    return vm_;
  }

  LRUCodeMap* direct_eval_map() {
    return &direct_eval_map_;
  }

  NativeIterator* GainNativeIterator(JSObject* obj);
  NativeIterator* GainNativeIterator(JSString* str);
  void ReleaseNativeIterator(NativeIterator* iterator);

  void Validate();  // for debug only

 private:
  NativeIterator* GainNativeIterator();
  VM* vm_;
  LRUCodeMap direct_eval_map_;
  std::vector<NativeIterator*> iterator_cache_;

#ifdef DEBUG
  int iterator_live_count_;
#endif
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONTEXT_FWD_H_
