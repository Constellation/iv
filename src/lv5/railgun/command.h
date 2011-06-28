#ifndef _IV_LV5_RAILGUN_COMMAND_H_
#define _IV_LV5_RAILGUN_COMMAND_H_
#include "lv5/railgun.h"
namespace iv {
namespace lv5 {
namespace railgun {

// some utility function for only railgun VM
inline JSVal StackDepth(const Arguments& args, Error* e) {
  const VM* vm = static_cast<Context*>(args.ctx())->vm();
  return std::distance(vm->stack()->GetBase(), vm->stack()->GetTop());
}


} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_COMMAND_H_
