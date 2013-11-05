#ifndef IV_LV5_BREAKER_FWD_H_
#define IV_LV5_BREAKER_FWD_H_

#include <iv/platform.h>
#include <iv/detail/cstdint.h>
#include <iv/lv5/jsval_fwd.h>

#if defined(IV_ENABLE_JIT)
// xbyak assembler
#include <iv/third_party/xbyak/xbyak.h>
namespace iv { namespace lv5 { namespace breaker {
using namespace Xbyak::util;  // NOLINT
} } }  // namespace iv::lv5::breaker
#endif  // defined(IV_ENABLE_JIT)

#include <iv/detail/cstdint.h>

#define IV_LV5_BREAKER_TO_STRING_IMPL(s) #s
#define IV_LV5_BREAKER_TO_STRING(s) IV_LV5_BREAKER_TO_STRING_IMPL(s)

// #define IV_LV5_BREAKER_CONST_IMPL(s) IV_LV5_BREAKER_TO_STRING($ ##s)
// #define IV_LV5_BREAKER_CONST(s) IV_LV5_BREAKER_CONST_IMPL(s)

// Because mangling convension in C is different,
// we must define mangling macro for each systems.
#if defined(IV_OS_MACOSX)
  #define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(_ ##sym)
  #define IV_LV5_BREAKER_HIDDEN(sym) ".private_extern " IV_LV5_BREAKER_SYMBOL(sym)
  #define IV_LV5_BREAKER_WEAK(sym) ".weak_definition " IV_LV5_BREAKER_SYMBOL(sym)
#elif defined(IV_OS_LINUX)
  #define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(sym)
  #define IV_LV5_BREAKER_HIDDEN(sym) ".hidden " IV_LV5_BREAKER_SYMBOL(sym)
  #define IV_LV5_BREAKER_WEAK(sym) ".weak " IV_LV5_BREAKER_SYMBOL(sym)
#else
  #define IV_LV5_BREAKER_SYMBOL(sym) IV_LV5_BREAKER_TO_STRING(sym)
  #define IV_LV5_BREAKER_HIDDEN(sym) ".hidden " IV_LV5_BREAKER_SYMBOL(sym)
  #define IV_LV5_BREAKER_WEAK(sym) ".weak " IV_LV5_BREAKER_SYMBOL(sym)
#endif

#if defined(IV_COMPILER_GCC)
  #define IV_LV5_BREAKER_ASM_DIRECTIVE __asm__ __volatile__
#else
  #define IV_LV5_BREAKER_ASM_DIRECTIVE __asm__
#endif

#define IV_LV5_BREAKER_ASM_HEADER(s)\
    ".text""\n"\
    ".globl " IV_LV5_BREAKER_SYMBOL(s) "\n"\
    IV_LV5_BREAKER_WEAK(s) "\n"\
    ".align 4" "\n"\
    IV_LV5_BREAKER_SYMBOL(s) ":"

#define IV_LV5_BREAKER_ASM_DEFINE(ret, name, args)\
    extern "C" ret name args;\
    IV_LV5_BREAKER_ASM_DIRECTIVE(IV_LV5_BREAKER_ASM_HEADER(name));\
    IV_LV5_BREAKER_ASM_DIRECTIVE

namespace iv {
namespace lv5 {

class Arguments;
class Error;
class JSEnv;

namespace railgun {

class Context;
struct Frame;
class Code;
class JSVMFunction;
struct Instruction;

}  // namespace railgun
namespace breaker {

static const int k64Size = sizeof(uint64_t);  // NOLINT
static const int k32Size = sizeof(uint32_t);  // NOLINT
static const int kJSValSize = sizeof(JSVal);  // NOLINT

class Context;
class Compiler;
class Assembler;
class NativeCode;
class JSJITFunction;
class IC;
class TemplatesGenerator;

class GlobalIC;
class LoadPropertyIC;
class StorePropertyIC;
class StoreElementIC;

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
IV_ALWAYS_INLINE Rep Extract(JSVal val) {
  return val.Layout().bytes_;
}

// Representation of paired return value
struct RepPair {
  uint64_t rax;
  uint64_t rdx;
};

template<typename T, typename U>
IV_ALWAYS_INLINE RepPair Extract(T first, U second) {
  const RepPair pair = {
    core::BitCast<uint64_t>(first),
    core::BitCast<uint64_t>(second)
  };
  return pair;
}

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
              JSJITFunction* func, Error* e);
void Compile(Context* ctx, railgun::Code* code);

JSVal FunctionConstructor(const Arguments& args, Error* e);
JSVal GlobalEval(const Arguments& args, Error* e);
JSVal DirectCallToEval(const Arguments& args, railgun::Frame* frame, Error* e);

typedef int32_t register_t;

namespace stub {

Rep LOAD_GLOBAL(Frame* stack, Symbol name, GlobalIC* ic, Assembler* as);

Rep STORE_GLOBAL(Frame* stack, Symbol name,
                 GlobalIC* ic, Assembler* as, JSVal src);

Rep LOAD_PROP_GENERIC(Frame* stack, JSVal base, LoadPropertyIC* site);
Rep LOAD_PROP(Frame* stack, JSVal base, LoadPropertyIC* site);

Rep STORE_PROP(Frame* stack, JSVal base, JSVal src, StorePropertyIC* ic);
Rep STORE_PROP_GENERIC(Frame* stack, JSVal base, JSVal src, StorePropertyIC* ic);  // NOLINT

Rep STORE_ELEMENT(Frame* stack, JSVal base, JSVal src, JSVal element, StoreElementIC* ic);  // NOLINT
Rep STORE_ELEMENT_GENERIC(Frame* stack, JSVal base, JSVal src, JSVal element, StoreElementIC* ic);  // NOLINT
Rep STORE_ELEMENT_INDEXED(Frame* stack, JSVal base, JSVal src, int32_t index, StoreElementIC* ic);  // NOLINT

} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_FWD_H_
