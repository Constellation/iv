#ifndef IV_LV5_RAILGUN_CONTEXT_FWD_H_
#define IV_LV5_RAILGUN_CONTEXT_FWD_H_
#include <gc/gc_cpp.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/lru_code_map.h>
#include <iv/lv5/factory.h>
#include <iv/lv5/error.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Context : public lv5::Context {
 public:
  friend class breaker::Compiler;
  static const std::size_t kNativeIteratorCacheMax = 20;
  static const std::size_t kGlobalMapCacheSize = 1024;
  typedef std::pair<Map*, Symbol> MapCacheKey;
  typedef std::pair<MapCacheKey, uint32_t> MapCacheEntry;
  typedef std::array<MapCacheEntry, kGlobalMapCacheSize> MapCache;

  Context();
  Context(JSAPI function_constructor, JSAPI global_eval);

  ~Context();

  VM* vm() {
    return vm_;
  }

  LRUCodeMap* direct_eval_map() {
    return &direct_eval_map_;
  }

  MapCache* global_map_cache() {
    return global_map_cache_;
  }

  virtual JSFunction* NewFunction(Code* code, JSEnv* env);

  inline JSVal& RAX() { return RAX_; }

  NativeIterator* GainNativeIterator(JSObject* obj);
  NativeIterator* GainNativeIterator(JSString* str);
  void ReleaseNativeIterator(NativeIterator* iterator);

  void Validate();  // for debug only

 private:
  void Init();

  NativeIterator* GainNativeIterator();
  VM* vm_;
  JSVal RAX_;
  LRUCodeMap direct_eval_map_;
  std::vector<NativeIterator*> iterator_cache_;
  MapCache* global_map_cache_;

#ifdef DEBUG
  int iterator_live_count_;
#endif
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONTEXT_FWD_H_
