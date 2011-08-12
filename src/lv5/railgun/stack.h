// railgun vm stack
// construct Frame on this stack,
// and traverse Frames when GC maker comes
#ifndef _IV_LV5_RAILGUN_STACK_H_
#define _IV_LV5_RAILGUN_STACK_H_
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <new>
#include "noncopyable.h"
#include "singleton.h"
#include "os_allocator.h"
#include "lv5/internal.h"
#include "lv5/jsval.h"
#include "lv5/gc_hook.h"
#include "lv5/railgun/frame.h"
#include "lv5/railgun/direct_threading.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Stack : core::Noncopyable<Stack> {
 public:
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

  // not bytes. JSVals capacity.
  // if you calc bytes, sizeof(JSVal) * kStackCapacity
  static const size_type kStackCapacity = 16 * 1024;
  static const size_type kStackBytes = kStackCapacity * sizeof(JSVal);

  // bytes. 4KB is page size.
  static const size_type kCommitSize = 4 * core::Size::KB;

  class Resource : public GCHook<Resource> {
   public:
    explicit Resource(Stack* stack)
      : stack_(stack) {
    }

    Stack* stack() const {
      return stack_;
    }

    GC_ms_entry* MarkChildren(GC_word* top,
                              GC_ms_entry* entry,
                              GC_ms_entry* mark_sp_limit,
                              GC_word env) {
      if (stack_) {
        Frame* current = stack_->current_;
        if (current) {
          // mark Frame member
          entry = MarkFrame(entry, mark_sp_limit,
                            current, stack_->stack_pointer_);
          // traverse frames
          for (Frame *next = current, *now = current->prev_;
               now; next = now, now = next->prev_) {
            entry = MarkFrame(entry, mark_sp_limit, now, next->GetFrameBase());
          }
        }
      }
      return entry;
    }

   private:
    Stack* stack_;
  };

  Stack()
    : stack_(NULL),
      stack_pointer_(NULL),
      resource_(NULL),
      base_(NULL),
      current_(NULL) {
    stack_pointer_ = stack_ =
        reinterpret_cast<JSVal*>(
            core::OSAllocator::Allocate(kStackBytes));
    stack_pointer_ += 1;  // for Global This
    // register root
    resource_ = new Resource(this);
  }

  explicit Stack(DispatchTableTag tag) { }  // empty

  ~Stack() {
    if (stack_) {  // normal pass
      core::OSAllocator::Decommit(stack_, kStackBytes);
      core::OSAllocator::Deallocate(stack_, kStackBytes);
      GC_free(resource_);
    }
  }

  // returns new frame for function call
  Frame* NewCodeFrame(Context* ctx,
                      JSVal* sp,
                      Code* code,
                      JSEnv* env,
                      Instruction* pc,
                      std::size_t argc,
                      bool constructor_call) {
    assert(code);
    if (JSVal* mem = GainFrame(sp, code)) {
      Frame* frame = reinterpret_cast<Frame*>(mem);
      frame->code_ = code;
      frame->prev_pc_ = pc;
      if (code->HasDeclEnv()) {
        frame->variable_env_ = frame->lexical_env_ =
            JSDeclEnv::New(ctx, env,
                           code->scope_nest_count(),
                           code->reserved_record_size());
      } else {
        frame->variable_env_ = frame->lexical_env_ = env;
      }
      frame->prev_ = current_;
      frame->ret_ = JSUndefined;
      frame->argc_ = argc;
      frame->dynamic_env_level_ = 0;
      frame->localc_ = code->locals().size();
      std::fill_n(frame->GetLocal(), frame->localc_, JSUndefined);
      frame->constructor_call_ = constructor_call;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  Frame* NewEvalFrame(Context* ctx,
                      JSVal* sp,
                      Code* code,
                      JSEnv* variable_env,
                      JSEnv* lexical_env) {
    assert(code);
    if (JSVal* mem = GainFrame(sp, code)) {
      Frame* frame = reinterpret_cast<Frame*>(mem);
      frame->code_ = code;
      frame->prev_pc_ = NULL;
      frame->variable_env_ = variable_env;
      frame->lexical_env_ = lexical_env;
      frame->prev_ = current_;
      frame->ret_ = JSUndefined;
      frame->argc_ = 0;
      frame->dynamic_env_level_ = 0;
      frame->localc_ = 0;
      frame->constructor_call_ = false;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  Frame* NewGlobalFrame(Context* ctx, Code* code) {
    assert(code);
    if (JSVal* mem = GainFrame(stack_ + 1, code)) {
      Frame* frame = reinterpret_cast<Frame*>(mem);
      frame->code_ = code;
      frame->prev_pc_ = NULL;
      frame->variable_env_ = frame->lexical_env_ = ctx->global_env();
      frame->prev_ = NULL;
      frame->ret_ = JSUndefined;
      frame->argc_ = 0;
      frame->dynamic_env_level_ = 0;
      frame->localc_ = 0;
      frame->constructor_call_ = false;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  void SetThis(JSVal val) {
    *stack_ = val;
  }

  Frame* Unwind(Frame* frame) {
    assert(current_ == frame);
    assert(frame->code());
    SetSafeStackPointerForFrame(frame, frame->prev_);
    current_ = frame->prev_;
    return current_;
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

  pointer GetTop() {
    return stack_pointer_;
  }

  const_pointer GetTop() const {
    return stack_pointer_;
  }

  const_pointer GetBase() const {
    return stack_;
  }

  // these 2 function Gain / Release is reserved for ScopedArguments
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

  Frame* current() {
    return current_;
  }

 private:
  static GC_ms_entry* MarkFrame(GC_ms_entry* entry,
                                GC_ms_entry* mark_sp_limit,
                                Frame* frame, JSVal* last) {
    entry = GC_MARK_AND_PUSH(frame->code_,
                             entry, mark_sp_limit,
                             reinterpret_cast<void**>(&frame));
    entry = GC_MARK_AND_PUSH(frame->lexical_env_,
                             entry, mark_sp_limit,
                             reinterpret_cast<void**>(&frame));
    entry = GC_MARK_AND_PUSH(frame->variable_env_,
                             entry, mark_sp_limit,
                             reinterpret_cast<void**>(&frame));
    if (frame->ret_.IsPtr()) {
      void* ptr = frame->ret_.pointer();
      entry = GC_MARK_AND_PUSH(ptr,
                               entry, mark_sp_limit,
                               reinterpret_cast<void**>(&frame));
    }

    // start current frame marking
    for (JSVal *it = frame->GetLocal(); it != last; ++it) {
      if (it->IsPtr()) {
        void* ptr = it->pointer();
        entry = GC_MARK_AND_PUSH(ptr,
                                 entry, mark_sp_limit,
                                 reinterpret_cast<void**>(&frame));
      }
    }
    return entry;
  }

  void SetSafeStackPointerForFrame(Frame* prev, Frame* current) {
    if (current) {
      JSVal* frame_last = current->GetFrameEnd();
      JSVal* prev_first = prev->GetFrameBase();
      stack_pointer_ = std::max(frame_last, prev_first);
    } else {
      // previous of Global Frame is NULL
      stack_pointer_ = stack_ + 1;
    }
  }

  JSVal* GainFrame(JSVal* top, Code* code) {
    assert(stack_ < top);
    assert(top <= stack_pointer_);
    stack_pointer_ = top;
    return Gain(Frame::GetFrameSize(code->stack_depth()));
  }

  // stack_pointer_ is safe point

  JSVal* stack_;
  JSVal* stack_pointer_;
  Resource* resource_;
  Frame* base_;
  Frame* current_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_STACK_H_
