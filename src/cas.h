#ifndef IV_CAS_H_
#define IV_CAS_H_
#include "platform.h"

#if defined(IV_OS_WIN)
#include "cas_win.h"
#elif defined(IV_OS_MACOSX)
#include "cas_mac.h"
#else
#include "cas_posix.h"
#endif  // IV_OS_WIN

#endif  // IV_CAS_H_
