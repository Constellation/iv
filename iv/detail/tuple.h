#ifndef IV_DETAIL_TUPLE_H_
#define IV_DETAIL_TUPLE_H_
#include <iv/platform.h>

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <tuple>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

// G++ 4.6 workaround. including <list> before using namespace tr1
#include <list>
#include <tr1/tuple>
namespace std { using namespace tr1; }

#endif

#endif  // IV_DETAIL_TUPLE_H_
