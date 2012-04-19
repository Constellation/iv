#ifndef IV_LV5_BREAKER_ASSEMBLER_H_
#define IV_LV5_BREAKER_ASSEMBLER_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/noncopyable.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Assembler : public Xbyak::CodeGenerator {
 public:
  class LocalLabelScope : core::Noncopyable<> {
   public:
    explicit LocalLabelScope(Assembler* assembler)
      : assembler_(assembler) {
      assembler_->inLocalLabel();
    }
    ~LocalLabelScope() {
      assembler_->outLocalLabel();
    }
   private:
    Assembler* assembler_;
  };

  Assembler()
    : Xbyak::CodeGenerator(4096, Xbyak::AutoGrow) {
  }

  // implementation of INT $3 and INT imm8
  // INT3 is used for gdb breakpoint.
  void interrupt(int imm8 = 3) {
    if (imm8 == 3) {
      db(Xbyak::B11001100);
    } else {
      db(Xbyak::B11001101);
      db(imm8);
    }
  }

  void Breakpoint() {
    interrupt();
  }

  void* GainExecutableByOffset(std::size_t offset) const {
    return core::BitCast<void*>(getCode() + offset);
  }

  template<typename Func>
  void Call(Func* f) {
    mov(rax, core::BitCast<uintptr_t>(f));
    call(rax);
  }

  template<typename Func, typename T1>
  void Call(Func* f, const T1& t1) {
    mov(rdi, t1);
    Call(f);
  }

  template<typename Func,
           typename T1, typename T2>
  void Call(Func* f, const T1& t1, const T2& t2) {
    mov(rsi, t2);
    Call(f, t1);
  }

  template<typename Func,
           typename T1, typename T2, typename T3>
  void Call(Func* f, const T1& t1, const T2& t2, const T3& t3) {
    mov(rdx, t3);
    Call(f, t1, t2);
  }

  template<typename Func,
           typename T1, typename T2, typename T3,
           typename T4>
  void Call(Func* f, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
    mov(rcx, t4);
    Call(f, t1, t2, t3);
  }

  template<typename Func,
           typename T1, typename T2, typename T3,
           typename T4, typename T5>
  void Call(Func* f,
            const T1& t1, const T2& t2,
            const T3& t3, const T4& t4, const T5& t5) {
    mov(r8, t5);
    Call(f, t1, t2, t3, t4);
  }

  template<typename Func,
           typename T1, typename T2, typename T3,
           typename T4, typename T5, typename T6>
  void Call(Func* f,
            const T1& t1, const T2& t2, const T3& t3,
            const T4& t4, const T5& t5, const T6& t6) {
    mov(r9, t6);
    Call(f, t1, t2, t3, t4, t5);
  }

  std::size_t size() const { return getSize(); }
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_ASSEMBLER_H_
