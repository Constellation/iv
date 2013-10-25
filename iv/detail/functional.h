#ifndef IV_DETAIL_FUNCTIONAL_H_
#define IV_DETAIL_FUNCTIONAL_H_
#include <iv/platform.h>

#if defined(IV_COMPILER_MSVC) || \
    defined(__GXX_EXPERIMENTAL_CXX0X__) || \
    (defined(IV_COMPILER_CLANG) && IV_COMPILER_CLANG >= 50000)
#include <functional>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else
// G++ 4.6 workaround. including <list> before using namespace tr1
#include <list>
#include <tr1/functional>
namespace std { using namespace tr1; }
#endif

#if defined(IV_COMPILER_MSVC) || \
    defined(__GXX_EXPERIMENTAL_CXX0X__) || \
    (defined(IV_COMPILER_CLANG) && IV_COMPILER_CLANG >= 50000)
#define IV_HASH_NAMESPACE_START std
#define IV_HASH_NAMESPACE_END
#else
#define IV_HASH_NAMESPACE_START std { namespace tr1
#define IV_HASH_NAMESPACE_END }
#endif

#endif  // IV_DETAIL_FUNCTIONAL_H_
