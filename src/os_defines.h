#ifndef _IV_OS_DEFINES_H_
#define _IV_OS_DEFINES_H_

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
#endif

#endif  // _IV_OS_DEFINES_H_
