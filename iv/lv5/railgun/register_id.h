#ifndef IV_LV5_RAILGUN_REGISTER_ID_H_
#define IV_LV5_RAILGUN_REGISTER_ID_H_
#include <iv/thread_safe_ref_counted.h>
#include <iv/intrusive_ptr.h>
#include <iv/sorted_vector.h>
namespace iv {
namespace lv5 {
namespace railgun {

// register layout
//                                 0
// ...[arg2][arg1][this][frame....][stack....][heap....][temp....]
class Registers {
 public:
  typedef core::SortedVector<int32_t, std::greater<int32_t> > Pool;

  class RegisterIDImpl : public core::ThreadSafeRefCounted<RegisterIDImpl> {
   public:
    enum Type {
      TEMPORARY = 0,
      STACK = 1,
      HEAP = 2,
      CONSTANT = 3
    };

    static const int kTypeMask = 3;

    explicit RegisterIDImpl(int32_t reg, Registers* holder)
      : reg_(reg),
        type_(holder->Type(reg_)),
        holder_(holder) { }

    ~RegisterIDImpl() {
      holder_->Release(register_offset());
    }

    bool IsStack() const { return (type_ & kTypeMask) == STACK; }

    bool IsHeap() const { return (type_ & kTypeMask) == HEAP; }

    bool IsTemporary() const { return (type_ & kTypeMask) == TEMPORARY; }

    bool IsLocal() const { return IsStack() || IsHeap(); }

    bool IsConstant() const { return (type_ & kTypeMask) == CONSTANT; }

    // these are defined in frame.h
    bool IsArgument() const;

    bool IsThis() const;

    int32_t register_offset() const { return reg_; }
   private:

    int32_t reg_;
    int type_;
    Registers* holder_;
  };

  friend class RegisterIDImpl;
  typedef core::IntrusivePtr<RegisterIDImpl> ID;

  Registers()
    : heap_(),
      stack_(),
      variable_registers_(),
      temporary_registers_(),
      lives_() {
  }

  void Clear(int32_t stack, int32_t heap) {
    lives_.clear();
    temporary_registers_.clear();
    heap_ = 0;  // currently not using heap
    // heap_ = heap;
    stack_ = stack;
    variable_registers_ = stack_ + heap_;
  }

  ID Acquire();

  ID Acquire(int32_t reg);

  ID LocalID(int32_t id);

  // implemented in frame.h

  ID This();

  // implemented in frame.h

  ID Callee();

  // implemented in frame.h

  ID Constant(uint32_t offset);

  int32_t AcquireSequence(int size);

  void Reserve(int32_t reg);

  int32_t size() const {
    return lives_.size();
  }

  bool IsHeapID(int32_t reg) {
    return stack_ <= reg && reg < variable_registers_;
  }

  bool IsStackID(int32_t reg) {
    return reg >= 0 && reg < stack_;
  }

  // implemented in frame.h

  bool IsConstantID(int32_t reg);

  // implemented in frame.h

  bool IsTemporaryID(int32_t reg);

  // implemented in frame.h

  static bool IsThisID(int32_t reg);

  // implemented in frame.h

  static bool IsCalleeID(int32_t reg);

  // implemented in frame.h

  static bool IsArgumentID(int32_t reg);

  int Type(uint32_t reg) {
    if (IsHeapID(reg)) {
      return RegisterIDImpl::HEAP;
    } else if (IsStackID(reg) || IsArgumentID(reg)) {
      return RegisterIDImpl::STACK;
    } else if (IsTemporaryID(reg)) {
      return RegisterIDImpl::TEMPORARY;
    } else {
      // IsConstantID or this or callee
      assert(IsThisID(reg) || IsConstantID(reg) || IsCalleeID(reg));
      return RegisterIDImpl::CONSTANT;
    }
  }

  // for debug in Compiler::EmitCall
  bool IsLiveTop(int32_t reg) {
    int32_t pos = lives_.size() + variable_registers_ - 1;
    for (std::vector<bool>::const_reverse_iterator it = lives_.rbegin(),
         last = lives_.rend(); it != last; ++it, --pos) {
      if (*it) {  // live register
        return pos == reg;
      }
    }
    return false;
  }

  int32_t stack_size() const { return stack_; }

  int32_t heap_size() const { return heap_; }

 private:
  void Release(int32_t reg) {
    if (IsTemporaryID(reg)) {
      // this is temporary register
      assert(lives_[reg - variable_registers_]);
      assert(std::find(
              temporary_registers_.begin(),
              temporary_registers_.end(),
              reg) == temporary_registers_.end());
      temporary_registers_.insert(reg);
      lives_[reg - variable_registers_] = false;
    }
  }

  int32_t heap_;
  int32_t stack_;
  int32_t variable_registers_;
  Pool temporary_registers_;
  std::vector<bool> lives_;
};

typedef Registers::ID RegisterID;

inline RegisterID Registers::Acquire() {
  if (temporary_registers_.empty()) {
    const int32_t n = lives_.size();
    lives_.push_back(true);
    return RegisterID(new RegisterIDImpl(n + variable_registers_, this), false);
  } else {
    const int32_t reg = temporary_registers_.back();
    temporary_registers_.pop_back();
    lives_[reg - variable_registers_] = true;
    return RegisterID(new RegisterIDImpl(reg, this), false);
  }
}

inline RegisterID Registers::Acquire(int32_t reg) {
  const Pool::iterator it = std::find(temporary_registers_.begin(),
                                      temporary_registers_.end(), reg);
  assert(it != temporary_registers_.end());
  temporary_registers_.erase(it);
  lives_[reg - variable_registers_] = true;
  return RegisterID(new RegisterIDImpl(reg, this), false);
}

inline int32_t Registers::AcquireSequence(int size) {
  if (temporary_registers_.empty()) {
    const int32_t reg = lives_.size() + variable_registers_;
    lives_.resize(lives_.size() + size, true);
    for (int i = 0; i < size; ++i) {
      Release(reg + i);
    }
    return reg;
  } else {
    int32_t offset = lives_.size();
    int require = size;
    for (std::vector<bool>::const_reverse_iterator it = lives_.rbegin(),
         last = lives_.rend(); it != last; ++it) {
      if (*it) {
        break;
      }
      if (!require) {
        break;
      }
      --require;
      --offset;
    }
    int32_t live_size = lives_.size();
    lives_.resize(offset + size, true);
    for (int32_t len = lives_.size(); live_size < len; ++live_size) {
      Release(live_size + variable_registers_);
    }
    return offset + variable_registers_;
  }
}

inline RegisterID Registers::LocalID(int32_t id) {
  assert(id < variable_registers_);
  return RegisterID(new RegisterIDImpl(id, this), false);
}

inline bool operator==(RegisterID lhs, RegisterID rhs) {
  if (lhs && rhs) {
    return lhs->register_offset() == rhs->register_offset();
  }
  return lhs == rhs;
}

inline bool operator!=(RegisterID lhs, RegisterID rhs) {
  return !(lhs == rhs);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_REGISTER_ID_H_
