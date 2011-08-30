#ifndef IV_DETAIL_UNORDERED_MAP_H_
#define IV_DETAIL_UNORDERED_MAP_H_
#include "platform.h"

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <unordered_map>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

// G++ 4.6 patching... including list before tr1/unordered_map
#include <list>
#include <tr1/unordered_map>
namespace std { using namespace tr1; }

#endif

#endif  // IV_DETAIL_UNORDERED_MAP_H_
