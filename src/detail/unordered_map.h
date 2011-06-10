#ifndef _IV_DETAIL_UNORDERED_MAP_H_
#define _IV_DETAIL_UNORDERED_MAP_H_
#include "platform.h"

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <unordered_map>

#if !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

#include <tr1/unordered_map>
namespace std { using namespace tr1; }

#endif

#endif  // _IV_DETAIL_UNORDERED_MAP_H_
