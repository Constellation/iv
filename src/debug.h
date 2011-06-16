#ifndef _IV_DEBUG_H_
#define _IV_DEBUG_H_
#include <cassert>

#ifdef NDEBUG
#undef DEBUG
#else
#ifndef DEBUG
#define DEBUG
#endif
#endif

#endif  // _IV_DEBUG_H_
