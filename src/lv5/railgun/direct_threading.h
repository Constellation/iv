#ifndef _IV_LV5_RAILGUN_DIRECT_THREADING_H_
#define _IV_LV5_RAILGUN_DIRECT_THREADING_H_
#include "platform.h"

#if defined(IV_USE_DIRECT_THREADED_CODE) && (defined(IV_COMPILER_GCC) || defined(IV_COMPILER_CLANG))  // NOLINT
#define IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE
#endif

#endif  // _IV_LV5_RAILGUN_DIRECT_THREADING_H_
