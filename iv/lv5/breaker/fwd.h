#ifndef IV_LV5_BREAKER_FWD_H_
#define IV_LV5_BREAKER_FWD_H_

#include <iv/platform.h>
#include <iv/static_assert.h>

#if defined(IV_ENABLE_JIT)
// xbyak assembler
#include <iv/third_party/xbyak/xbyak.h>
#endif  // defined(IV_ENABLE_JIT)

#include <iv/detail/cstdint.h>

// Even if -fomit-frame-pointer is provided, __builtin_frame_address works fine
// GCC 4.0 or later.
// So, we restrict GCC version >= 4.
//
// See http://www.mail-archive.com/gcc@gcc.gnu.org/msg32003.html
#define IV_LV5_BREAKER_RETURN_ADDRESS() __builtin_return_address(0)
#define IV_LV5_BREAKER_RETURN_ADDRESS_POSITION\
    ((void**)(((uint64_t*)__builtin_frame_address(0)) + 1))  /* NOLINT */

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

#define IV_LV5_BREAKER_TO_STRING_IMPL(s) #s
#define IV_LV5_BREAKER_TO_STRING(s) IV_LV5_BREAKER_TO_STRING_IMPL(s)

// #define IV_LV5_BREAKER_CONST_IMPL(s) IV_LV5_BREAKER_TO_STRING($ ##s)
// #define IV_LV5_BREAKER_CONST(s) IV_LV5_BREAKER_CONST_IMPL(s)

// Because mangling convension in C is different,
// we must define mangling macro for each systems.
#if defined(IV_OS_MACOSX)
  #define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(_ ##sym)
#elif defined(IV_OS_LINUX)
  #define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(sym)
#else
  #define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(sym)
#endif

#define IV_LV5_BREAKER_RAISE()\
  do {\
    IV_LV5_BREAKER_ASSERT_RETURN_ADDRESS();\
    void* pc = IV_LV5_BREAKER_RETURN_ADDRESS();\
    IV_LV5_BREAKER_REPATCH_RETURN_ADDRESS(Templates<>::dispatch_exception_handler());  /* NOLINT */\
    return core::BitCast<uint64_t>(pc);\
  } while (0)

namespace iv {
namespace lv5 {
namespace railgun {

class Context;
struct Frame;
class Code;

}  // namespace railgun
namespace breaker {

static const int k64Size = sizeof(uint64_t);  // NOLINT

class Compiler;
class Assembler;

// Representation of JSVal, it is uint64_t in 64bit system
typedef uint64_t Rep;

Rep Extract(JSVal val);
void* search_exception_handler(void* pc, railgun::Context* ctx, void** target);
JSVal breaker_prologue(railgun::Context* ctx, railgun::Frame* frame, void* ptr);
JSVal Run(railgun::Context* ctx, railgun::Code* code, Error* e);

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_FWD_H_
