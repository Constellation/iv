// breaker::Compiler
//
// This compiler parses railgun::opcodes and emit native code.
// Some primitive operation and branch operation is emitted as raw native code,
// and basic complex opcode is emitted as call to stub function, that is,
// Context Threading JIT.
//
// Stub function implementations are in stub.h
#ifndef IV_LV5_BREAKER_COMPILER_H_
#define IV_LV5_BREAKER_COMPILER_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Compiler {
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_H_
