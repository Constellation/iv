#ifndef _IV_LV5_STACK_RESOURCE_H_
#define _IV_LV5_STACK_RESOURCE_H_
#include <new>
#include <gc/gc.h>
extern "C" {
#include <gc/gc_mark.h>
}
#include "noncopyable.h"
#include "lv5/vm_stack.h"
namespace iv {
namespace lv5 {

class StackResource : private core::Noncopyable<StackResource>::type {
 public:
  typedef struct GC_ms_entry GCMSEntry;

  StackResource()
    : stack_free_list_(GC_new_free_list()),
      stack_mark_proc_(GC_new_proc(&StackMark)),
      stack_kind_(GC_new_kind(stack_free_list_,
                              GC_MAKE_PROC(stack_mark_proc_, 0), 0, 0)),
      stack_(NULL) {
    stack_ = new (GC_generic_malloc(sizeof(VMStack), stack_kind_)) VMStack();
  }

  ~StackResource() {
    stack_->~VMStack();
    GC_free(stack_);
  }

  VMStack* stack() const {
    return stack_;
  }

  static GCMSEntry* StackMark(GC_word* stack,
                              GCMSEntry* mark_sp,
                              GCMSEntry* mark_sp_limit,
                              GC_word env) {
    // first JSVal is pointer to VMStack
    const VMStack* vmstack = reinterpret_cast<VMStack*>(stack);

    GCMSEntry* entry = mark_sp;

    for (VMStack::iterator it = vmstack->stack_pointer_begin(),
         last = vmstack->stack_pointer(); it != last; ++it) {
      if (it->IsPtr()) {
        void* ptr = it->pointer();
        if (GC_least_plausible_heap_addr < ptr &&
            ptr < GC_greatest_plausible_heap_addr) {
          entry = GC_mark_and_push(ptr,
                                   entry, mark_sp_limit,
                                   reinterpret_cast<void**>(&stack));
        }
      }
    }
    return entry;
  }

 private:
  void** stack_free_list_;
  int stack_mark_proc_;
  int stack_kind_;
  VMStack* stack_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_STACK_RESOURCE_H_
