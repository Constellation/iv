#ifndef IV_LV5_BREAKER_FWD_H_
#define IV_LV5_BREAKER_FWD_H_

#include <iv/platform.h>
#include <iv/static_assert.h>
#include <iv/detail/cstdint.h>

#if defined(IV_ENABLE_JIT)
// xbyak assembler
#include <iv/third_party/xbyak/xbyak.h>
#endif  // defined(IV_ENABLE_JIT)

#include <iv/detail/cstdint.h>

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

namespace iv {
namespace lv5 {

class Arguments;

namespace railgun {

class Context;
struct Frame;
class Code;
class JSVMFunction;

}  // namespace railgun
namespace breaker {

static const int k64Size = sizeof(uint64_t);  // NOLINT
static const int k32Size = sizeof(uint32_t);  // NOLINT

class Context;
class Compiler;
class MonoIC;
class Assembler;
class JSFunction;
class IC;

// JIT Frame layout. This frame layout is constructed on breaker prologue
struct Frame {
  void* r12;
  void* r13;
  void* r14;
  void* r15;
  void** rsp;
  Context* ctx;
  railgun::Frame* frame;
  Error* error;
  void** ret;
};

// Representation of JSVal, it is uint64_t in 64bit system
typedef uint64_t Rep;

Rep Extract(JSVal val);
void* SearchExceptionHandler(void* pc, void** rsp,
                             Frame* stack, railgun::Frame* frame);
JSVal breaker_prologue(Context* ctx,
                       railgun::Frame* frame, void* ptr, Error* e);
JSVal RunEval(Context* ctx,
              railgun::Code* code,
              JSEnv* variable_env,
              JSEnv* lexical_env,
              JSVal this_binding,
              Error* e);
JSVal Run(Context* ctx, railgun::Code* code, Error* e);
JSVal Execute(Context* ctx, Arguments* args,
              JSFunction* func, Error* e);
void Compile(Context* ctx, railgun::Code* code);

JSVal FunctionConstructor(const Arguments& args, Error* e);
JSVal GlobalEval(const Arguments& args, Error* e);
JSVal DirectCallToEval(const Arguments& args, railgun::Frame* frame, Error* e);

typedef int32_t register_t;

namespace stub {

template<bool Strict>
Rep LOAD_GLOBAL(Frame* stack, Symbol name, MonoIC* ic, Assembler* as);

template<bool Strict>
Rep STORE_GLOBAL(Frame* stack, Symbol name,
                 MonoIC* ic, Assembler* as, JSVal src);

} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_FWD_H_
