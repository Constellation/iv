#ifndef IV_CAS_H_
#define IV_CAS_H_
#include <iv/platform.h>

#if defined(IV_OS_WIN)
#include <iv/cas_win.h>
#elif defined(IV_OS_MACOSX)
#include <iv/cas_mac.h>
#else
#include <iv/cas_posix.h>
#endif  // IV_OS_WIN

#endif  // IV_CAS_H_
