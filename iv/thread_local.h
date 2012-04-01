#ifndef IV_THREAD_LOCAL_H_
#define IV_THREAD_LOCAL_H_
#include <iv/platform.h>

#if defined(IV_OS_WIN)
#include <iv/thread_local_win.h>
#else
#include <iv/thread_local_posix.h>
#endif  // defined(IV_OS_WIN)

#endif  // IV_THREAD_LOCAL_H_
