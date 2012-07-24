// MonoIC compiler
#ifndef IV_BREAKER_MONO_IC_H_
#define IV_BREAKER_MONO_IC_H_
#include <iv/lv5/breaker/assembler.h>
namespace iv {
namespace lv5 {
namespace breaker {

// MonoIC emits following x86_64 assembly code
//
//
// ENTRANCE:
//   mov rax, map
//   cmp obj[map], rax
//   jne CALLING
//   mov rax, obj[offset]
//   jmp EXIT
//
// CALLING:
//   call function
//
// EXIT:
//
//
// Once we extract map pointer and offset from calling function,
// we'll repatch pointer and offset in inline cache.
class MonoIC {
 public:
  MonoIC(Assembler* assembler)
    : position_(assembler->size()) {
  }

  void Emit(Assembler* assembler) {
  }

  void Repatch(Map* map) {
    // Repatch map pointer in machine code
  }

  Map* Extract() const {
    // Extract map pointer from code for GC.
    return NULL;
  }

 private:
  std::size_t position() const {
    return position_;
  }
  std::size_t position_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
