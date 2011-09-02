#ifndef IV_CALLONCE_H_
#define IV_CALLONCE_H_
#include "platform.h"

typedef void(*OnceCallback)(void);

#if defined(IV_OS_WIN)
#include <windows.h>
#include "callonce_win.h"
#else
#include "callonce_posix.h"
#endif
#endif  // IV_CALLONCE_H_
