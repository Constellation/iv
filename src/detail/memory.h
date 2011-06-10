#ifndef _IV_DETAIL_MEMORY_H_
#define _IV_DETAIL_MEMORY_H_
#include "platform.h"

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <memory>

#if !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

#include <tr1/memory>
namespace std { using namespace tr1; }

#endif

#endif  // _IV_DETAIL_MEMORY_H_
