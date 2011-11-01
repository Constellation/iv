#ifndef IV_CALLONCE_H_
#define IV_CALLONCE_H_
#include <iv/platform.h>

typedef void(*OnceCallback)(void);

#if defined(IV_OS_WIN)
#include <windows.h>
#include <iv/callonce_win.h>
#else
#include <iv/callonce_posix.h>
#endif
#endif  // IV_CALLONCE_H_
