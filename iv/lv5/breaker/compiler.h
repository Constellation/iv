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
 public:
  Compiler()
    : code_(NULL) {
  }

  void Initialize(railgun::Code* code) {
    code_ = code;
  }

  void Compile(railgun::Code* code) {
    Initialize(code);
  }
};

inline void CompileInternal(Compiler* compiler, railgun::Code* code) {
  compiler->Compile(code);
  for (Code::Codes::const_iterator it = code->codes().begin(),
       last = code->codes().end(); it != last; ++it) {
    CompileInternal(compiler, *it);
  }
}

inline void Compile(railgun::Code* code) {
  Compiler compiler;
  CompileInternal(compiler, code);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_H_
