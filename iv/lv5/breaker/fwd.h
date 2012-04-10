#ifndef IV_LV5_BREAKER_FWD_H_
#define IV_LV5_BREAKER_FWD_H_

#include <iv/platform.h>

#if defined(IV_ENABLE_JIT)
// xbyak assembler
#include <iv/third_party/xbyak/xbyak.h>
#endif

#include <iv/detail/cstdint.h>

namespace iv {
namespace lv5 {
namespace breaker {

class Compiler;

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_FWD_H_
