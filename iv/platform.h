#ifndef IV_PLATFORM_H_
#define IV_PLATFORM_H_

#if (defined(__unix__) || defined(unix)) && !defined(USG)
  #include <sys/param.h>
#endif

#ifndef IV_MAKE_VERSION
  #define IV_MAKE_VERSION(major, minor, patch)\
      ((major) * 10000 + (minor) * 100 + (patch))
#endif

#if defined(_WIN32) || defined(_WIN64)
  #ifndef IV_OS_WIN
    #define IV_OS_WIN 1
    #define NOMINMAX
  #endif  // IV_OS_WIN
#elif defined(__APPLE__) || defined(__darwin__)
  #ifndef IV_OS_MACOSX
    #define IV_OS_MACOSX 1
  #endif  // IV_OS_MACOSX
#elif defined(sun) || defined(__sun)
  #ifndef IV_OS_SOLARIS
    #define IV_OS_SOLARIS 1
  #endif  // IV_OS_SOLARIS
#elif defined(__linux__)
  #ifndef IV_OS_LINUX
    #define IV_OS_LINUX 1
  #endif  // IV_OS_LINUX
#elif defined(BSD)
  #ifndef IV_OS_BSD
    #define IV_OS_BSD 1
  #endif  // IV_OS_BSD
#elif defined(__CYGWIN__)
  #ifndef IV_OS_CYGWIN
    #define IV_OS_CYGWIN 1
  #endif  // IV_OS_CYGWIN
#endif

// compiler
#if defined(__clang__)
  #ifndef IV_COMPILER_CLANG
    #if defined(__clang_patchlevel__)
      #define IV_COMPILER_CLANG\
        IV_MAKE_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
    #else
      #define IV_COMPILER_CLANG IV_MAKE_VERSION(__clang_major__, __clang_minor__, 0)
    #endif
  #endif
#elif defined(__ICL) || defined(__ICC)
  #ifndef IV_COMPILER_ICC
    #if defined(__ICL)
      #define IV_COMPILER_ICC __ICL
    #else
      #define IV_COMPILER_ICC __ICC
    #endif
  #endif
#elif defined(_MSC_VER)
  #ifndef IV_COMPILER_MSVC
    #define IV_COMPILER_MSVC _MSC_VER
    #define IV_COMPILER_MSVC_10 (_MSC_VER >= 1600)
  #endif
#elif defined(__GNUC__)
  #ifndef IV_COMPILER_GCC
    #if defined(__GNU_PATCHLEVEL__)
      #define IV_COMPILER_GCC\
        IV_MAKE_VERSION(__GNUC__, __GNUC_MINOR__, __GNU_PATCHLEVEL__)
    #else
      #define IV_COMPILER_GCC IV_MAKE_VERSION(__GNUC__, __GNUC_MINOR__, 0)
    #endif
  #endif
#endif

// snprintf
#if defined(IV_COMPILER_MSVC)
  #ifndef snprintf
    #define snprintf _snprintf_s
  #endif
#endif

// 64bit system or not
#ifndef IV_64
  #if defined(__LP64__)
    #define IV_64
  #elif defined(_WIN64)
    #define IV_64
  #endif
#endif

// CPU
#if defined(__x86_64__) || defined(_WIN64)
  #ifndef IV_CPU_X64
    #define IV_CPU_X64
  #endif
#elif defined(__i386__)
  #ifndef IV_CPU_IA32
    #define IV_CPU_IA32
  #endif
#elif defined(__arm__)
  #ifndef IV_CPU_ARM
    #define IV_CPU_ARM
  #endif
#else
  #ifndef IV_CPU_OTHERS
    #define IV_CPU_OTHERS
  #endif
#endif

// SSE
#if defined(__SSE1__) || (defined(_M_IX86_FP) && _M_IX86_FP == 1)
  #undef IV_CPU_SSE
  #define IV_CPU_SSE IV_MAKE_VERSION(1, 0, 0)
#endif

#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP == 2)
  #undef IV_CPU_SSE
  #define IV_CPU_SSE IV_MAKE_VERSION(2, 0, 0)
#endif

// Currenly, we cannot detect AVX archtecture on MSVC
//
// http://connect.microsoft.com/VisualStudio/feedback/details/605858/arch-avx-should-define-a-predefined-macro-in-x64-and-set-a-unique-value-for-m-ix86-fp-in-win32
#if defined(__SSE3__) || (defined(_M_IX86_FP) && _M_IX86_FP == 3)
  #undef IV_CPU_SSE
  #define IV_CPU_SSE IV_MAKE_VERSION(3, 0, 0)
#endif

#if defined(__SSE4_1__) || (defined(_M_IX86_FP) && _M_IX86_FP == 3)
  #undef IV_CPU_SSE
  #define IV_CPU_SSE IV_MAKE_VERSION(4, 1, 0)
#endif

#if defined(__SSE4_2__)
  #undef IV_CPU_SSE
  #define IV_CPU_SSE IV_MAKE_VERSION(4, 2, 0)
#endif

// ENABLE_JIT
// http://homepage1.nifty.com/herumi/diary/1109.html#7
#if not +0
  #ifndef IV_DISABLE_JIT
    #define IV_DISABLE_JIT
  #endif
// #error "operator names used. use -fno-operator-names"
#endif

#if !defined(IV_DISABLE_JIT)
  #if defined(IV_CPU_X64) &&\
      (defined(IV_OS_MACOSX) || defined(IV_OS_LINUX) || defined(IV_OS_BSD))
    #if defined(IV_COMPILER_CLANG) ||\
      (defined(IV_COMPILER_GCC) && IV_COMPILER_GCC >= IV_MAKE_VERSION(4, 0, 0))
      #define IV_ENABLE_JIT
    #endif
  #endif
#endif

#if !defined(IV_ENABLE_JIT)
  #ifndef IV_DISABLE_JIT
    #define IV_DISABLE_JIT
  #endif
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) ||\
    (defined(IV_COMPILER_MSVC) && defined(IV_COMPILER_MSVC_10))
  #ifndef IV_CXX11
    #define IV_CXX11
  #endif
#endif

#ifndef IV_UNALIGNED_WORD_ACCESS
  #if defined(__i386) || defined(__i386__) || defined(_M_IX86) || \
      defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD86) || \
      defined(__mc68020__)
    #define IV_UNALIGNED_WORD_ACCESS 1
  #endif
#endif

#if defined(IV_COMPILER_GCC) || defined(IV_COMPILER_CLANG)
  #define IV_ALWAYS_INLINE inline __attribute__((always_inline))
  #define IV_NEVER_INLINE __attribute__((noinline))
#elif defined(IV_COMPILER_MSVC)
  #define IV_ALWAYS_INLINE __forceinline
  #define IV_NEVER_INLINE
#else
  #define IV_ALWAYS_INLINE inline
  #define IV_NEVER_INLINE
#endif

#endif  // IV_PLATFORM_H_
