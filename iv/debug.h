#ifndef IV_DEBUG_H_
#define IV_DEBUG_H_
#include <cassert>

#ifdef NDEBUG
#undef DEBUG
#else
#ifndef DEBUG
#define DEBUG
#endif
#endif

// GDB breakpoint macro
// See http://0xcc.net/blog/archives/000100.html
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && __GNUC__ >= 2
#define IV_DEBUG_BREAKPOINT()\
  do {\
    __asm__ __volatile__("int $03");\
  } while (0)
#elif defined (_MSC_VER) && defined (_M_IX86)
#define IV_DEBUG_BREAKPOINT()\
  do {\
    __asm int 3h\
  } while (0)
#elif defined(__alpha__) && !defined(__osf__) && defined(__GNUC__) && __GNUC__ >= 2
#define IV_DEBUG_BREAKPOINT()\
  do {\
    __asm__ __volatile__("bpt");\
  } while (0)
#else   /* !__i386__ && !__alpha__ */
#define IV_DEBUG_BREAKPOINT()\
  do {\
    raise(SIGTRAP);\
  } while (0)
#endif  /* __i386__ */

#endif  // IV_DEBUG_H_
