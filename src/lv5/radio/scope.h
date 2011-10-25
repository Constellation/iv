#ifndef IV_LV5_RADIO_SCOPE_H_
#define IV_LV5_RADIO_SCOPE_H_
#include "noncopyable.h"
#include "lv5/radio/core_fwd.h"
namespace iv {
namespace lv5 {
namespace radio {

class Scope : private core::Noncopyable<Scope> {
 public:
  Scope(Core* core)
    : core_(core) {
    core_->EnterScope(this);
  }

  ~Scope() {
    core_->ExitScope(this);
  }

 private:
  Core* core_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_H_
