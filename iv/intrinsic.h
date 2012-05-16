#ifndef IV_INTRINSIC_H_
#define IV_INTRINSIC_H_

#include <iv/platform.h>

#if defined(IV_CPU_X64) || defined(IV_CPU_IA32)
  #if defined(IV_COMPILER_MSVC)
    // See MSVC http://msdn.microsoft.com/ja-jp/library/bb892950.aspx
    #include <intrin.h>
    #define IV_INTRINSIC_AVAILABLE
  #elif defined(IV_COMPILER_CLANG) || \
        (defined(IV_COMPILER_GCC) && \
         IV_COMPILER_GCC >= IV_MAKE_VERSION(4, 5, 0))
    // See http://gcc.gnu.org/gcc-4.5/changes.html
    #include <x86intrin.h>
    #define IV_INTRINSIC_AVAILABLE
  #endif
#endif

#endif  // IV_INTRINSIC_H_
