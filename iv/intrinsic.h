#ifndef IV_INTRINSIC_H_
#define IV_INTRINSIC_H_

#include <iv/platform.h>

#if defined(IV_CPU_X64) || defined(IV_CPU_IA32)
  #if defined(IV_COMPILER_MSVC)
    // See MSVC http://msdn.microsoft.com/ja-jp/library/bb892950.aspx
    #include <intrin.h>
    #define IV_INTRINSIC_AVAILABLE
  #elif defined(IV_COMPILER_CLANG) || defined(IV_COMPILER_GCC)
    #include <x86intrin.h>
    #define IV_INTRINSIC_AVAILABLE
  #endif
#endif

#endif  // IV_INTRINSIC_H_
