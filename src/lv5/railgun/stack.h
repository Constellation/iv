// railgun vm stack
// construct Frame on this stack,
// and traverse Frames when GC maker comes
#ifndef _IV_LV5_RAILGUN_STACK_H_
#define _IV_LV5_RAILGUN_STACK_H_
#include <cstddef>
#include <algorithm>
#include <iterator>
#include <gc/gc.h>
extern "C" {
#include <gc/gc_mark.h>
}
#include "noncopyable.h"
#include "noncopyable.h"
#include "os_allocator.h"
#include "lv5/jsval.h"
#include "lv5/railgun/frame.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Stack : core::Noncopyable<Stack> {
 public:
  typedef JSVal* iterator;
  typedef const JSVal* const_iterator;
  typedef struct GC_ms_entry GCMSEntry;

  typedef std::iterator_traits<iterator>::value_type value_type;

  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  // not bytes. JSVals capacity.
  // if you calc bytes, sizeof(JSVal) * kStackCapacity
  static const size_type kStackCapacity = 16 * 1024;
  static const size_type kStackBytes = kStackCapacity * sizeof(JSVal);

  // bytes. 4KB is page size.
  static const size_type kCommitSize = 4 * 1024;

  Stack()
    : stack_(NULL),
      stack_pointer_(NULL),
      stack_free_list_(GC_new_free_list()),
      stack_mark_proc_(GC_new_proc(&Mark)),
      stack_kind_(GC_new_kind(stack_free_list_,
                              GC_MAKE_PROC(stack_mark_proc_, 0), 0, 0)),
      stack_for_gc_(NULL),
      base_(NULL),
      current_(NULL) {
    stack_pointer_ = stack_ =
        reinterpret_cast<JSVal*>(
            core::OSAllocator::Allocate(kStackBytes));
    // register root
    stack_for_gc_ =
        reinterpret_cast<Stack**>(
            GC_generic_malloc(sizeof(Stack*), stack_kind_));  // NOLINT
    *stack_for_gc_ = this;
  }

  ~Stack() {
    core::OSAllocator::Decommit(stack_, kStackBytes);
    core::OSAllocator::Deallocate(stack_, kStackBytes);
    GC_free(stack_for_gc_);
  }

  // returns new frame for function call
  Frame* NewCodeFrame(Context* ctx,
                      Code* code,
                      JSEnv* env,
                      const uint8_t* pc,
                      std::size_t argc) {
    assert(code);
    if (JSVal* mem = Gain(
            Frame::GetFrameSize(code->stack_depth()) / sizeof(JSVal))) {
      Frame* frame = reinterpret_cast<Frame*>(mem);
      frame->code_ = code;
      frame->prev_pc_ = pc;
      frame->variable_env_ = frame->lexical_env_ =
          internal::NewDeclarativeEnvironment(ctx, env);
      frame->prev_ = current_;
      frame->argc_ = argc;
      frame->dynamic_env_level_ = 0;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  Frame* NewGlobalFrame(Context* ctx, Code* code) {
    assert(code);
    if (JSVal* mem = Gain(
            Frame::GetFrameSize(code->stack_depth()) / sizeof(JSVal))) {
      Frame* frame = reinterpret_cast<Frame*>(mem);
      frame->code_ = code;
      frame->prev_pc_ = 0;
      frame->variable_env_ = frame->lexical_env_ = ctx->global_env();
      frame->prev_ = NULL;
      frame->argc_ = 0;
      frame->dynamic_env_level_ = 0;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  Frame* Unwind(Frame* frame) {
    current_ = frame->prev_;
    assert(frame->code());
    Release(Frame::GetFrameSize(frame->code()->stack_depth()) / sizeof(JSVal));
    return current_;
  }

  static GCMSEntry* Mark(GC_word* top,
                         GCMSEntry* mark_sp,
                         GCMSEntry* mark_sp_limit,
                         GC_word env) {
    Stack* stack = *reinterpret_cast<Stack**>(top);
    Frame* current = stack->current_;

    GCMSEntry* entry = mark_sp;

    // mark Frame member
    entry = GC_mark_and_push(current->code_,
                             entry, mark_sp_limit,
                             reinterpret_cast<void**>(&current));
    entry = GC_mark_and_push(current->lexical_env_,
                             entry, mark_sp_limit,
                             reinterpret_cast<void**>(&current));
    entry = GC_mark_and_push(current->variable_env_,
                             entry, mark_sp_limit,
                             reinterpret_cast<void**>(&current));
    if (current->ret_.IsPtr()) {
      entry = GC_mark_and_push(current->ret_.pointer(),
                               entry, mark_sp_limit,
                               reinterpret_cast<void**>(&current));
    }

    // start current frame marking
    for (JSVal *it = current->GetStackBase(),
         *last = stack->stack_pointer_; it != last; ++it) {
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

    // traverse frames
    for (Frame *next = current, *now = current->prev_;
         now; next = now, now = next->prev_) {
      entry = GC_mark_and_push(now->code_,
                               entry, mark_sp_limit,
                               reinterpret_cast<void**>(&now));
      entry = GC_mark_and_push(now->lexical_env_,
                               entry, mark_sp_limit,
                               reinterpret_cast<void**>(&now));
      entry = GC_mark_and_push(now->variable_env_,
                               entry, mark_sp_limit,
                               reinterpret_cast<void**>(&now));
      if (now->ret_.IsPtr()) {
        entry = GC_mark_and_push(now->ret_.pointer(),
                                 entry, mark_sp_limit,
                                 reinterpret_cast<void**>(&now));
      }

      // and mark stacks
      for (JSVal *it = now->GetStackBase(),
           *last = next->GetFrameBase(); it != last; ++it) {
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
    }
    return entry;
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
    if (stack_pointer_ + n < end()) {
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

  JSVal* stack_;
  JSVal* stack_pointer_;

  void** stack_free_list_;
  int stack_mark_proc_;
  int stack_kind_;
  Stack** stack_for_gc_;

  Frame* base_;
  Frame* current_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_STACK_H_
