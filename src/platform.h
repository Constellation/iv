#ifndef _IV_PLATFORM_H_
#define _IV_PLATFORM_H_

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#if defined(WIN32) || defined(WIN64)
#ifndef OS_WIN
#define OS_WIN 1
#endif  // OS_WIN
#elif defined(__APPLE__) || defined(__darwin__)
#ifndef OS_MACOSX
#define OS_MACOSX 1
#endif  // OS_MACOSX
#elif defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX 1
#endif  // OS_LINUX
#elif defined(BSD)
#ifndef OS_BSD
#define OS_BSD 1
#endif  // OS_BSD
#elif defined(__CYGWIN__)
#ifndef OS_CYGWIN
#define OS_CYGWIN 1
#endif  // OS_CYGWIN
#endif

#endif  // _IV_PLATFORM_H_
