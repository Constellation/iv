#ifndef _IV_LV5_VMSTACK_H_
#define _IV_LV5_VMSTACK_H_
#include <iostream>
#include <iterator>
#include <gc/gc.h>
extern "C" {
#include <gc/gc_mark.h>
}
#include "noncopyable.h"
#include "lv5/jsval.h"
namespace iv {
namespace lv5 {

class VMStack : private core::Noncopyable<VMStack>::type {
 public:
  typedef struct GC_ms_entry GCMSEntry;

  typedef VMStack this_type;
  typedef JSVal* iterator;
  typedef const JSVal* const_iterator;

  typedef std::iterator_traits<iterator>::value_type value_type;

  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  static const size_type kStackCapacity = 16 * 1024;

  VMStack()
    : stack_free_list_(GC_new_free_list()),
      stack_mark_proc_(GC_new_proc(&StackMark)),
      stack_kind_(GC_new_kind(stack_free_list_,
                              GC_MAKE_PROC(stack_mark_proc_, 0), 0, 0)),
      stack_(NULL),
      stack_pointer_base_(NULL),
      stack_pointer_(NULL) {
    void* mem = GC_generic_malloc(kStackCapacity * sizeof(JSVal), stack_kind_);
    *reinterpret_cast<VMStack**>(mem) = this;
    stack_ = reinterpret_cast<JSVal*>(mem);
    stack_pointer_base_ = stack_pointer_ = stack_ + 1;
  }

  static GCMSEntry* StackMark(GC_word* stack_start,
                              GCMSEntry* mark_sp,
                              GCMSEntry* mark_sp_limit,
                              GC_word env) {
    // first JSVal is pointer to VMStack
    const VMStack* vmstack = *reinterpret_cast<VMStack**>(stack_start);

    GCMSEntry* entry = mark_sp;

    const void* const stack_pointer_begin = reinterpret_cast<void*>(vmstack->stack_pointer_base());
    const void* const stack_pointer_end = reinterpret_cast<void*>(vmstack->stack_pointer_end());
    const void* const heap_begin = GC_least_plausible_heap_addr;
    const void* const heap_end = GC_greatest_plausible_heap_addr;

    for (iterator it = vmstack->stack_pointer_base(),
         last = vmstack->stack_pointer(); it != last; ++it) {
      if (it->IsPtr()) {
        void* ptr = it->pointer();
        if ((heap_begin < ptr && ptr < stack_pointer_begin) ||
            (stack_pointer_end < ptr && ptr < heap_end)) {
          entry = GC_mark_and_push(ptr,
                                   entry, mark_sp_limit,
                                   reinterpret_cast<void**>(&stack_start));
        }
      }
    }
    return entry;
  }

  pointer stack_pointer_base() const {
    return stack_pointer_base_;
  }

  pointer stack_pointer_end() const {
    return stack_ + kStackCapacity;
  }

  pointer stack_pointer() const {
    return stack_pointer_;
  }

  const_iterator begin() const {
    return stack_;
  }

  const_iterator end() const {
    return stack_ + kStackCapacity;
  }

  iterator begin() {
    return stack_;
  }

  iterator end() {
    return stack_ + kStackCapacity;
  }

  pointer Gain(size_type n) {
    if (stack_pointer_ + n < stack_pointer_end()) {
      const pointer stack = stack_pointer_;
      stack_pointer_ += n;
      return stack;
    } else {
      // stack over flow
      return NULL;
    }
  }

  void Release(size_type n) {
    stack_pointer_ -= n;
  }

 private:
  void** stack_free_list_;
  int stack_mark_proc_;
  int stack_kind_;
  pointer stack_;
  pointer stack_pointer_base_;
  pointer stack_pointer_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_VMSTACK_H_
