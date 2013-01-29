// railgun vm stack
// construct Frame on this stack,
// and traverse Frames when GC maker comes
#ifndef IV_LV5_RAILGUN_STACK_H_
#define IV_LV5_RAILGUN_STACK_H_
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <new>
#include <iv/noncopyable.h>
#include <iv/singleton.h>
#include <iv/os_allocator.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/gc_kind.h>
#include <iv/lv5/stack.h>
#include <iv/lv5/railgun/frame.h>
#include <iv/lv5/railgun/direct_threading.h>
#include <iv/lv5/radio/core_fwd.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Stack : public lv5::Stack {
 public:
  friend class breaker::Compiler;
  class Resource : public GCKind<Resource> {
   public:
    explicit Resource(Stack* stack) : stack_(stack) { }

    Stack* stack() const { return stack_; }

    GC_ms_entry* MarkChildren(GC_word* top,
                              GC_ms_entry* entry,
                              GC_ms_entry* mark_sp_limit,
                              GC_word env) {
      if (stack_) {
        Frame* current = stack_->current_;
        if (current) {
          // mark Frame member
          entry = MarkFrame(entry, mark_sp_limit,
                            current, stack_->GetTop());
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
    : lv5::Stack(),
      resource_(NULL),
      current_(NULL) {
    resource_ = new Resource(this);
  }

  explicit Stack(DispatchTableTag tag)
    : lv5::Stack(lv5::Stack::EmptyTag()) { }  // empty

  ~Stack() {
    if (GetBase()) {
      delete resource_;
    }
  }

  // returns new frame for function call
  Frame* NewCodeFrame(Context* ctx,
                      JSVal* arg,
                      Code* code,
                      JSEnv* env,
                      JSVal callee,
                      Instruction* pc,
                      std::size_t argc_with_this,
                      bool constructor_call) {
    assert(code);
    assert(argc_with_this >= 1);
    if (Frame* frame = GainFrame(arg, argc_with_this, code)) {
      frame->code_ = code;
      frame->prev_pc_ = pc;
      frame->variable_env_ = frame->lexical_env_ = env;
      frame->prev_ = current_;
      frame->callee_ = callee;
      frame->argc_ = argc_with_this - 1;
      frame->constructor_call_ = constructor_call;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  Frame* NewEvalFrame(Context* ctx,
                      JSVal* arg,
                      Code* code,
                      JSEnv* variable_env,
                      JSEnv* lexical_env) {
    assert(code);
    if (Frame* frame = GainFrame(arg, 1, code)) {
      frame->code_ = code;
      frame->prev_pc_ = NULL;
      frame->variable_env_ = variable_env;
      frame->lexical_env_ = lexical_env;
      frame->prev_ = current_;
      frame->callee_ = JSVal(JSUndefined);
      frame->argc_ = 0;
      frame->constructor_call_ = false;
      current_ = frame;
      return frame;
    } else {
      // stack overflow
      return NULL;
    }
  }

  Frame* Unwind(Frame* frame) {
    assert(current_ == frame);
    assert(frame->code());
    current_ = frame->prev_;
    if (current_) {
      // because of ScopedArguments
      Restore((std::max)(current_->GetFrameEnd(), frame->GetFrameBase()));
    } else {
      // previous of Global Frame is NULL
      Restore();
    }
    return current_;
  }

  Frame* current() { return current_; }

  void MarkChildren(radio::Core* core) {
    // mark Frame member
    MarkFrame(core, current_, GetTop());
    // traverse frames
    for (Frame *next = current_, *now = current_->prev_;
         now; next = now, now = next->prev_) {
      MarkFrame(core, now, next->GetFrameBase());
    }
  }

  static std::size_t CurrentFrameOffset() {
    return IV_OFFSETOF(Stack, current_);
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
    if (frame->callee_.IsCell()) {
      radio::Cell* ptr = frame->callee_.cell();
      entry = GC_MARK_AND_PUSH(ptr,
                               entry, mark_sp_limit,
                               reinterpret_cast<void**>(&frame));
    }

    // start current frame marking
    for (JSVal *it = frame->RegisterFile(); it != last; ++it) {
      if (it->IsCell()) {
        radio::Cell* ptr = it->cell();
        entry = GC_MARK_AND_PUSH(ptr,
                                 entry, mark_sp_limit,
                                 reinterpret_cast<void**>(&frame));
      }
    }
    return entry;
  }

  void MarkFrame(radio::Core* core, Frame* frame, JSVal* last) {
    core->MarkCell(frame->code_);
    core->MarkCell(frame->lexical_env_);
    core->MarkCell(frame->variable_env_);
    core->MarkValue(frame->callee_);
    std::for_each(frame->RegisterFile(), last, radio::Core::Marker(core));
  }

  // argc_with_this is this + args size
  Frame* GainFrame(JSVal* arg, int argc_with_this, Code* code) {
    const int diff =
        static_cast<int>(code->params().size()) - (argc_with_this - 1);
    const std::size_t capacity = Frame::GetFrameSize(code->FrameSize());
    JSVal* top = arg + argc_with_this;
    if (diff <= 0) {
      return reinterpret_cast<Frame*>(Gain(top, capacity));
    }

    // fill and move args
    JSVal* ptr = Gain(top, capacity + diff) + diff;
    std::copy_backward(arg, arg + argc_with_this, ptr);
    std::fill_n<JSVal*, std::size_t, JSVal>(arg, diff, JSUndefined);
    return reinterpret_cast<Frame*>(ptr);
  }

  Resource* resource_;
  Frame* current_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_STACK_H_
