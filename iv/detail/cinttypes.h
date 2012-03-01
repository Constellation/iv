#ifndef IV_DETAIL_CINTTYPES_H_
#define IV_DETAIL_CINTTYPES_H_
#include <iv/platform.h>

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#ifdef IV_COMPILER_MSVC
#include <stdint.h>
#define PRIu32 "u"
#define PRIu64 "llu"
#else
#include <cinttypes>
#endif

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

// G++ 4.6 workaround. including <list> before using namespace tr1
#include <list>
#include <tr1/cinttypes>
namespace std { using namespace tr1; }

#endif



#endif  // IV_DETAIL_CINTTYPES_H_
