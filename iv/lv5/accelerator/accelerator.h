#ifndef IV_LV5_ACCELERATOR_H_
#define IV_LV5_ACCELERATOR_H_

// exported by vm.S
void IvLv5AcceleratorMain();

namespace iv {
namespace lv5 {
namespace accelerator {

inline void Run() {
  IvLv5AcceleratorMain();
}

} } }  // namespace iv::lv5::accelerator
#endif  // IV_LV5_ACCELERATOR_H_
