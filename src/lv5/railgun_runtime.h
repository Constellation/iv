#ifndef _IV_LV5_RUNTIME_RAILGUN_H_
#define _IV_LV5_RUNTIME_RAILGUN_H_
#include "lv5/error_check.h"
#include "lv5/railgun_fwd.h"
#include "lv5/internal.h"
namespace iv {
namespace lv5 {
namespace railgun {

inline JSVal FunctionConstructor(const Arguments& args, Error* e) {
  return JSUndefined;
}

inline JSVal GlobalEval(const Arguments& args, Error* e) {
  return JSUndefined;
}

} } }  // iv::lv5::railgun
#endif  // _IV_LV5_RUNTIME_RAILGUN_H_
