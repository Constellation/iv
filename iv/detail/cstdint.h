#ifndef IV_DETAIL_CSTDINT_H_
#define IV_DETAIL_CSTDINT_H_
#include <iv/platform.h>

#if defined(IV_COMPILER_MSVC) || \
    defined(__GXX_EXPERIMENTAL_CXX0X__) || \
    (defined(IV_COMPILER_CLANG) && IV_COMPILER_CLANG >= IV_MAKE_VERSION(5, 0, 0))
#include <cstdint>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

// G++ 4.6 workaround. including <list> before using namespace tr1
#include <list>
#include <tr1/cstdint>
namespace std { using namespace tr1; }

#endif



#endif  // IV_DETAIL_CSTDINT_H_
