// MonoIC compiler
#ifndef IV_BREAKER_MONO_IC_H_
#define IV_BREAKER_MONO_IC_H_
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/ic.h>
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
class MonoIC : public IC, public Xbyak::CodeGenerator {
 public:
  static const int kICSize = 1024;
  static const int kMovImmOffset = 2;  // rex and code

  MonoIC()
    : IC(IC::MONO),
      Xbyak::CodeGenerator(kICSize),
      position_() {
  }

  void Compile(Assembler* as) {
    const uint64_t dummy = UINT64_C(0x0FFF000000000000);
    position_ = as->size() + kMovImmOffset;
    as->mov(as->rcx, dummy);
  }

  void Repatch(Assembler* as, Map* map, uint32_t offset) {
    // Repatch map pointer in machine code
    as->rewrite(position_, core::BitCast<uint64_t>(map), k64Size);
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
