#ifndef IV_NO_OPERATOR_NAMES_GUARD_H_
#define IV_NO_OPERATOR_NAMES_GUARD_H_
#include <iv/platform.h>
// http://homepage1.nifty.com/herumi/diary/1109.html#7

#if not +0
#define IV_DISABLE_JIT
// #error "operator names used. use -fno-operator-names"
#else
#if defined(IV_CPU_X64)
#if defined(IV_OS_MACOSX) || defined(IV_OS_LINUX) || defined(IV_OS_BSD)
#define IV_ENABLE_JIT
#endif
#endif
#endif

#endif  // IV_NO_OPERATOR_NAMES_GUARD_H_
