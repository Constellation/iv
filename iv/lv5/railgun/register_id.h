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
    temporary_registers_size_ = 0;
    temporary_registers_.clear();
    variable_registers_ = variable_registers;
  }

  ID Acquire();

  ID LocalID(uint32_t id);

  uint32_t size() const {
    return temporary_registers_size_;
  }

  bool IsLocalID(uint32_t reg) {
    return variable_registers_ > reg;
  }

 private:
  void Release(uint32_t reg) {
    if (!IsLocalID(reg)) {
      // this is temporary register
      temporary_registers_.insert(reg);
    }
  }

  uint32_t variable_registers_;
  uint32_t temporary_registers_size_;
  core::SortedVector<uint32_t, std::greater<uint32_t> > temporary_registers_;
};

typedef Registers::ID RegisterID;

inline RegisterID Registers::Acquire() {
  if (temporary_registers_.empty()) {
    const uint32_t n = temporary_registers_size_++;
    return RegisterID(new RegisterIDImpl(n + variable_registers_, this), false);
  } else {
    const uint32_t res = temporary_registers_.back();
    temporary_registers_.pop_back();
    return RegisterID(new RegisterIDImpl(res, this), false);
  }
}

inline RegisterID Registers::LocalID(uint32_t id) {
  assert(id < variable_registers_);
  return RegisterID(new RegisterIDImpl(id, this), false);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_REGISTER_ID_H_
