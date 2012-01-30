#ifndef IV_LV5_RAILGUN_REGISTER_ID_H_
#define IV_LV5_RAILGUN_REGISTER_ID_H_
#include <iv/thread_safe_ref_counted.h>
#include <iv/intrusive_ptr.h>
#include <iv/sorted_vector.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Registers {
 public:
  typedef core::SortedVector<uint32_t, std::greater<uint32_t> > Pool;

  class RegisterIDImpl : public core::ThreadSafeRefCounted<RegisterIDImpl> {
   public:
    explicit RegisterIDImpl(uint32_t reg, Registers* holder)
      : reg_(reg),
        holder_(holder) { }

    ~RegisterIDImpl() {
      holder_->Release(reg());
    }

    uint32_t reg() const { return reg_; }

    bool IsLocal() const { return holder_->IsLocalID(reg_); }
   private:
    uint32_t reg_;
    Registers* holder_;
  };

  friend class RegisterIDImpl;
  typedef core::IntrusivePtr<RegisterIDImpl> ID;

  void Clear(uint32_t variable_registers) {
    lives_.clear();
    temporary_registers_.clear();
    variable_registers_ = variable_registers;
  }

  ID Acquire();

  ID Acquire(uint32_t reg);

  ID LocalID(uint32_t id);

  uint32_t AcquireCallBase(int size);

  void Reserve(uint32_t reg);

  uint32_t size() const {
    return lives_.size();
  }

  bool IsLocalID(uint32_t reg) {
    return variable_registers_ > reg;
  }

  // for debug in Compiler::EmitCall
  bool IsLiveTop(uint32_t reg) {
    uint32_t pos = lives_.size() + variable_registers_ - 1;
    for (std::vector<bool>::const_reverse_iterator it = lives_.rbegin(),
         last = lives_.rend(); it != last; ++it, --pos) {
      if (*it) {  // live register
        return pos == reg;
      }
    }
    return false;
  }

 private:
  void Release(uint32_t reg) {
    if (!IsLocalID(reg)) {
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

  uint32_t variable_registers_;
  Pool temporary_registers_;
  std::vector<bool> lives_;
};

typedef Registers::ID RegisterID;

inline RegisterID Registers::Acquire() {
  if (temporary_registers_.empty()) {
    const uint32_t n = lives_.size();
    lives_.push_back(true);
    return RegisterID(new RegisterIDImpl(n + variable_registers_, this), false);
  } else {
    const uint32_t reg = temporary_registers_.back();
    temporary_registers_.pop_back();
    lives_[reg - variable_registers_] = true;
    return RegisterID(new RegisterIDImpl(reg, this), false);
  }
}

inline RegisterID Registers::Acquire(uint32_t reg) {
  const Pool::iterator it = std::find(temporary_registers_.begin(),
                                      temporary_registers_.end(), reg);
  assert(it != temporary_registers_.end());
  temporary_registers_.erase(it);
  lives_[reg - variable_registers_] = true;
  return RegisterID(new RegisterIDImpl(reg, this), false);
}

inline uint32_t Registers::AcquireCallBase(int size) {
  if (temporary_registers_.empty()) {
    const uint32_t reg = lives_.size() + variable_registers_;
    lives_.resize(lives_.size() + size, true);
    for (int i = 0; i < size; ++i) {
      Release(reg + i);
    }
    return reg;
  } else {
    uint32_t offset = lives_.size();
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
    uint32_t live_size = lives_.size();
    lives_.resize(offset + size, true);
    for (uint32_t len = lives_.size(); live_size < len; ++live_size) {
      Release(live_size + variable_registers_);
    }
    return offset + variable_registers_;
  }
}

inline RegisterID Registers::LocalID(uint32_t id) {
  assert(id < variable_registers_);
  return RegisterID(new RegisterIDImpl(id, this), false);
}

inline bool operator==(RegisterID lhs, RegisterID rhs) {
  if (lhs && rhs) {
    return lhs->reg() == rhs->reg();
  }
  return lhs == rhs;
}

inline bool operator!=(RegisterID lhs, RegisterID rhs) {
  return !(lhs == rhs);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_REGISTER_ID_H_
