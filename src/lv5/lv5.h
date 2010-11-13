#ifndef _IV_LV5_LV5_H_
#define _IV_LV5_LV5_H_
#include "jsval.h"
#define ERROR_WITH(error, val)\
  error);\
  if (*error) {\
    return val;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define ERROR(error)\
  ERROR_WITH(error, JSUndefined)

namespace iv {
namespace lv5 {
} }  // namespace iv::lv5
#endif  // _IV_LV5_LV5_H_
