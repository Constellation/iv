#ifndef IV_DETAIL_RANDOM_H_
#define IV_DETAIL_RANDOM_H_
// old g++ uniform_int has fatal bug
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=33128
// so make uniform_int by myself
#include <iv/platform.h>

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <random>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

// G++ 4.6 workaround. including <list> before using namespace tr1
#include <list>
#include <tr1/random>
namespace std { using namespace tr1; }

#endif

#endif  // IV_DETAIL_RANDOM_H_
