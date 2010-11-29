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

// msg must be string literal
#define CONSTRUCTOR_CHECK(msg, args, error)\
  do {\
    if (args.IsConstructorCalled()) {\
      error->Report(Error::Type,\
                    msg " is not a constructor");\
      return JSUndefined;\
    }\
  } while (0)

namespace iv {
namespace lv5 {
} }  // namespace iv::lv5
#endif  // _IV_LV5_LV5_H_
