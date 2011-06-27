#ifndef _IV_LV5_RAILGUN_CONTEXT_H_
#define _IV_LV5_RAILGUN_CONTEXT_H_
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/context_fwd.h"
#include "lv5/railgun/runtime.h"
namespace iv {
namespace lv5 {
namespace railgun {

Context::Context(VM* vm)
  : lv5::Context(),
    vm_(vm) {
  vm->set_context(this);
  Initialize<&FunctionConstructor, &GlobalEval>();
}

JSVal* Context::StackGain(std::size_t size) {
  return vm_->stack()->Gain(size);
}

void Context::StackRelease(std::size_t size) {
  vm_->stack()->Release(size);
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CONTEXT_H_
