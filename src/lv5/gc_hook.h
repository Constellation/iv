// BoehmGC original GC Kind maker
#ifndef IV_LV5_GC_HOOK_H_
#define IV_LV5_GC_HOOK_H_
#include <gc/gc.h>
extern "C" {
#include <gc/gc_mark.h>
}
#include "noncopyable.h"
#include "singleton.h"
namespace iv {
namespace lv5 {

template<typename Target>
class GCHook {
 public:
  class GCKind : public core::Singleton<GCKind> {
   public:
    friend class core::Singleton<GCKind>;

    int GetKind() const {
      return stack_kind_;
    }

   private:
    GCKind()
      : stack_kind_(
          GC_new_kind(GC_new_free_list(),
                      GC_MAKE_PROC(
                          GC_new_proc(&GCHook<Target>::Mark), 0), 0, 1)) {
    }

    ~GCKind() { }  // private destructor

    volatile int stack_kind_;
  };

  void* operator new(std::size_t size) {
    return GC_generic_malloc(size, GCKind::Instance()->GetKind());
  }

  void operator delete(void* obj) {
    GC_FREE(obj);
  }

  static GC_ms_entry* Mark(GC_word* top, GC_ms_entry* entry,
                           GC_ms_entry* mark_sp_limit, GC_word env) {
    return reinterpret_cast<Target*>(top)->MarkChildren(top,
                                                        entry,
                                                        mark_sp_limit, env);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GC_HOOK_H_
