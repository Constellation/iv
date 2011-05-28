#ifndef _IV_LV5_RAILGUN_CONTEXT_H_
#define _IV_LV5_RAILGUN_CONTEXT_H_
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/runtime.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Context : public lv5::Context {
 public:
  Context()
    : lv5::Context() {
    Initialize<&FunctionConstructor, &GlobalEval>();
  }
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CONTEXT_H_
