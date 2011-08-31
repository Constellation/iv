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

#endif  // IV_DEBUG_H_
