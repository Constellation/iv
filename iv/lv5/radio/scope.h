#ifndef IV_LV5_RADIO_SCOPE_H_
#define IV_LV5_RADIO_SCOPE_H_
#include <iv/noncopyable.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace radio {

class Scope : private core::Noncopyable<Scope> {
 public:
  friend class Core;
  explicit Scope(Core* core)
    : core_(core),
      reserved_(NULL),
      current_() {
    core_->EnterScope(this);
  }

  explicit Scope(Context* ctx) { }

  ~Scope() {
    core_->ExitScope(this);
  }

  std::size_t current() const {
    return current_;
  }

  JSVal Close(JSVal val) {
    reserved_ = val.IsCell() ? val.cell() : NULL;
    return val;
  }

  Cell* Close(Cell* cell) {
    return reserved_ = cell;
  }

  Cell* reserved() const {
    return reserved_;
  }

 private:
  void set_current(std::size_t current) {
    current_ = current;
  }

  Core* core_;
  Cell* reserved_;
  std::size_t current_;
};

#ifdef DEBUG
// for debug
class NoAllocationScope : private core::Noncopyable<NoAllocationScope> {
 public:
  friend class Core;
  explicit NoAllocationScope(Core* core)
    : core_(core) {
    core_->EnterScope(this);
  }

  ~NoAllocationScope() {
    core_->FenceScope(this);
  }

  std::size_t current() const {
    return current_;
  }
 private:
  void set_current(std::size_t current) {
    current_ = current;
  }

  Core* core_;
  std::size_t current_;
};

#else

class NoAllocationScope : private core::Noncopyable<NoAllocationScope> {
 public:
  friend class Core;
  explicit NoAllocationScope(Core* core) { }
};

#endif  // ifdef DEBUG

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_H_
