// BoehmGC original GC Kind maker
#ifndef IV_LV5_GC_KIND_H_
#define IV_LV5_GC_KIND_H_
#include <gc/gc.h>
extern "C" {
#include <gc/gc_mark.h>
}
#include <iv/detail/unordered_set.h>
#include <iv/noncopyable.h>
#include <iv/singleton.h>
namespace iv {
namespace lv5 {

template<typename Target>
class GCKind {
 public:
  typedef GCKind<Target> this_type;
  class GCKindProvider : public core::Singleton<GCKindProvider> {
   public:
    friend class core::Singleton<GCKindProvider>;

    int GetKind() const {
      return stack_kind_;
    }

    void* Malloc(std::size_t size) {
      void* mem = GC_generic_malloc(
          size, GCKindProvider::Instance()->GetKind());
      allocated_.insert(mem);
      GC_REGISTER_FINALIZER_IGNORE_SELF(mem,
                                        &DestructorCall,
                                        nullptr, nullptr, nullptr);
      return mem;
    }

    static void DestructorCall(void* obj, void* client_data) {
      static_cast<this_type*>(obj)->~this_type();
      GCKindProvider::Instance()->Remove(obj);
    }

    void Remove(void* ptr) {
      allocated_.erase(ptr);
    }

    bool IsAllocated(void* ptr) {
      return allocated_.find(ptr) != allocated_.end();
    }

   private:
    GCKindProvider()
      : allocated_(),
        stack_kind_(
          GC_new_kind(GC_new_free_list(),
                      GC_MAKE_PROC(
                          GC_new_proc(&GCKind<Target>::Mark), 0), 0, 1)) {
    }

    ~GCKindProvider() { }  // private destructor

    std::unordered_set<void*> allocated_;
    volatile int stack_kind_;
  };

  void* operator new(std::size_t size) {
    return GCKindProvider::Instance()->Malloc(size);
  }

  void operator delete(void* obj) {
    GC_FREE(obj);
    GC_REGISTER_FINALIZER_IGNORE_SELF(obj, nullptr, nullptr, nullptr, nullptr);
  }

  virtual ~GCKind() { }

  static GC_ms_entry* Mark(GC_word* top, GC_ms_entry* entry,
                           GC_ms_entry* mark_sp_limit, GC_word env) {
    if (top && GCKindProvider::Instance()->IsAllocated(top)) {
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
#endif  // IV_LV5_GC_KIND_H_
