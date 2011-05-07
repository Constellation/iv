#ifndef _IV_LV5_RAILGUN_CONTEXT_H_
#define _IV_LV5_RAILGUN_CONTEXT_H_
#include "lv5/railgun_fwd.h"
#include "lv5/railgun_runtime.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Context : public lv5::Context {
 public:
  Context()
    : lv5::Context() {
    Initialize(
        JSInlinedFunction<&FunctionConstructor, 1>::NewPlain(this));
  }
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CONTEXT_H_
