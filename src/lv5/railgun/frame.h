#ifndef _IV_LV5_RAILGUN_FRAME_H_
#define _IV_LV5_RAILGUN_FRAME_H_
#include <cstddef>
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Frame {
 public:
  friend class VM;
  friend class JSVMFunction;
  const Code& code() const {
    return *code_;
  }

  const uint8_t* data() const {
    return code_->data();
  }

  const JSVals& constants() const {
    return code_->constants();
  }

  JSVal* stacktop() const {
    return stacktop_;
  }

  JSEnv* env() const {
    return env_;
  }

  void set_this_binding(const JSVal& this_binding) {
    this_binding_ = this_binding;
  }

  const JSVal& this_binding() const {
    return this_binding_;
  }

 private:
  Code* code_;
  std::size_t lineno_;
  JSVal* stacktop_;
  JSEnv* env_;
  JSVal this_binding_;
  Frame* back_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_FRAME_H_
