// BoehmGC original GC Kind maker
#ifndef _IV_LV5_GC_HOOK_H_
#define _IV_LV5_GC_HOOK_H_
#include <gc/gc.h>
extern "C" {
#include <gc/gc_mark.h>
}
#include "detail/unordered_set.h"
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

    void* Malloc(std::size_t size) {
      void* mem = GC_generic_malloc(size, GCKind::Instance()->GetKind());
      allocated_.insert(mem);
      GC_REGISTER_FINALIZER_NO_ORDER(mem,
                                     RemoveSet,
                                     NULL,
                                     NULL, NULL);
      return mem;
    }

    static void RemoveSet(void* obj, void* client_data) {
      GCKind::Instance()->Remove(reinterpret_cast<void*>(obj));
    }

    void Remove(void* ptr) {
      allocated_.erase(ptr);
    }

    bool IsAllocated(void* ptr) {
      return allocated_.find(ptr) != allocated_.end();
    }

   private:
    GCKind()
      : allocated_(),
        stack_kind_(
          GC_new_kind(GC_new_free_list(),
                      GC_MAKE_PROC(
                          GC_new_proc(&GCHook<Target>::Mark), 0), 0, 1)) {
    }

    ~GCKind() { }  // private destructor

    std::unordered_set<void*> allocated_;
    volatile int stack_kind_;
  };

  void* operator new(std::size_t size) {
    return GCKind::Instance()->Malloc(size);
  }

  void operator delete(void* obj) {
    GC_REGISTER_FINALIZER_NO_ORDER(obj, NULL, NULL, NULL, NULL);
    GC_FREE(obj);
  }

  static GC_ms_entry* Mark(GC_word* top, GC_ms_entry* entry,
                           GC_ms_entry* mark_sp_limit, GC_word env) {
    if (top && GCKind::Instance()->IsAllocated(top)) {
      entry = reinterpret_cast<Target*>(top)->MarkChildren(top,
                                                          entry,
                                                          mark_sp_limit, env);
      return entry;
    } else {
      return entry;
    }
  }
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_GC_HOOK_H_
