#ifndef IV_LV5_RADIO_SCOPE_H_
#define IV_LV5_RADIO_SCOPE_H_
#include "noncopyable.h"
#include "lv5/jsval_fwd.h"
#include "lv5/radio/core_fwd.h"
namespace iv {
namespace lv5 {
namespace radio {

class Scope : private core::Noncopyable<Scope> {
 public:
  explicit Scope(Core* core)
    : core_(core),
      reserved_(NULL),
      current_() {
    core_->EnterScope(this);
  }

  ~Scope() {
    core_->ExitScope(this);
  }

  void set_current(std::size_t current) {
    current_ = current;
  }

  std::size_t current() const {
    return current_;
  }

  const JSVal& Close(const JSVal& val) {
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
  Core* core_;
  Cell* reserved_;
  std::size_t current_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_H_
