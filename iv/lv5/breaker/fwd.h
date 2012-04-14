#ifndef IV_LV5_BREAKER_FWD_H_
#define IV_LV5_BREAKER_FWD_H_

#include <iv/platform.h>

#if defined(IV_ENABLE_JIT)
// xbyak assembler
#include <iv/third_party/xbyak/xbyak.h>
#endif

#include <iv/detail/cstdint.h>

// Even if -fomit-frame-pointer is provided, __builtin_frame_address works fine
// GCC 4.0 or later.
// So, we restrict GCC version >= 4.
//
// See http://www.mail-archive.com/gcc@gcc.gnu.org/msg32003.html
#define IV_LV5_BREAKER_RETURN_ADDRESS() __builtin_return_address(0)
#define IV_LV5_BREAKER_RETURN_ADDRESS_POSITION\
    ((void**)(((uint64_t*)__builtin_frame_address(0)) + 1))

#define IV_LV5_BREAKER_REPATCH_RETURN_ADDRESS(ptr)\
  do {\
    *IV_LV5_BREAKER_RETURN_ADDRESS_POSITION = (ptr);\
  } while (0)

#define IV_LV5_BREAKER_ASSERT_RETURN_ADDRESS()\
  do {\
    assert(IV_LV5_BREAKER_RETURN_ADDRESS() ==\
           *IV_LV5_BREAKER_RETURN_ADDRESS_POSITION);\
  } while (0)

#define IV_NEVER_INLINE __attribute__((noinline))
#define IV_ALWAYS_INLINE __attribute__((always_inline))

#define IV_LV5_BREAKER_TO_STRING(s) #s


// Because mangling convension in C is different,
// we must define mangling macro for each systems.
#if defned(IV_OS_MACOSX)
#define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(_ ##sym)
#elif defined(IV_OS_LINUX)
#define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(sym)
#else
#error Unknown symbol convension. Please add to iv/lv5/breaker/fwd.h
#endif

#define IV_LV5_BREAKER_RAISE()\
  do {\
    IV_LV5_BREAKER_ASSERT_RETURN_ADDRESS();\
    void* pc = IV_LV5_BREAKER_RETURN_ADDRESS();\
    REPATCH_RETURN_ADDRESS(reinterpret_cast<void*>(&iv_lv5_breaker_dispatch_exception_handler));\
    return pc;\
  } while (0)

// prototype
extern "C" void iv_lv5_breaker_dispatch_exception_handler(void);
extern "C" void* iv_lv5_breaker_search_exception_handler(void* pc);
extern "C" void iv_lv5_breaker_exception_handler_is_not_found(void);

namespace iv {
namespace lv5 {
namespace breaker {

class Compiler;
class Assembler;

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_FWD_H_
