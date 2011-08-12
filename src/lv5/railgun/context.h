#ifndef _IV_LV5_RAILGUN_CONTEXT_H_
#define _IV_LV5_RAILGUN_CONTEXT_H_
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/context_fwd.h"
#include "lv5/railgun/runtime.h"
namespace iv {
namespace lv5 {
namespace railgun {

Context::Context()
  : lv5::Context(),
    vm_() {
  void* mem = GC_MALLOC_UNCOLLECTABLE(sizeof(VM));
  vm_ = new(mem)VM();
  vm_->set_context(this);
  Initialize<&FunctionConstructor, &GlobalEval>();
}

Context::~Context() {
  vm_->~VM();
  GC_FREE(static_cast<void*>(vm_));
}

JSVal* Context::StackGain(std::size_t size) {
  return vm_->stack()->Gain(size);
}

void Context::StackRelease(std::size_t size) {
  vm_->stack()->Release(size);
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CONTEXT_H_
