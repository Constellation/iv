#ifndef IV_LV5_RUNTIME_FWD_H_
#define IV_LV5_RUNTIME_FWD_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal ObjectFreeze(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_FWD_H_
