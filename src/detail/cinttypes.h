#ifndef IV_DETAIL_CINTTYPES_H_
#define IV_DETAIL_CINTTYPES_H_
#include "platform.h"

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <cinttypes>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

#include <tr1/cinttypes>
namespace std { using namespace tr1; }

#endif



#endif  // IV_DETAIL_CINTTYPES_H_
