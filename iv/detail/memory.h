#ifndef IV_DETAIL_MEMORY_H_
#define IV_DETAIL_MEMORY_H_
#include <iv/platform.h>

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <memory>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

#include <tr1/memory>
namespace std { using namespace tr1; }

#endif

#endif  // IV_DETAIL_MEMORY_H_
