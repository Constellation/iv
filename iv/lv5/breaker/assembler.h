#ifndef IV_LV5_BREAKER_ASSEMBLER_H_
#define IV_LV5_BREAKER_ASSEMBLER_H_
#include <iv/detail/memory.h>
#include <iv/noncopyable.h>
#include <iv/xbyak_mmap_allocator.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/ic.h>
#include <iv/lv5/breaker/executable_pages.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Assembler
  : private core::XbyakMMapAllocator
  , public Xbyak::CodeGenerator {
 public:
  friend class Compiler;
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

  class RepatchSite {
   public:
    static const std::size_t kMovImmOffset = 2;  // rex and code

    // repatch offset from call return address
    //
    // mov rax, $REPATCH
    // call rax
    // <RET ADDR>    -- from there to $REPATCH
    //
    // call rax is 2bytes, (FF D0)
    // $REPATCH is 8bytes
    //
    // so offset is 2 + 8 = 10
    static const std::size_t kRepatchOffset = 10;

    RepatchSite() : offset_(0) { }

    void Mov(Assembler* assembler, const Xbyak::Reg64& reg) {
      // not int32 point
      const uint64_t dummy = UINT64_C(0x0FFF000000000000);
      offset_ = assembler->size() + kMovImmOffset;
      assembler->mov(reg, dummy);
    }

    void MovRepatchableAligned(Assembler* assembler, const Xbyak::Reg64& reg) {
      // not int32 point
      const uint64_t dummy = UINT64_C(0x0FFF000000000000);
      while (reinterpret_cast<uintptr_t>(assembler->getCurr()) % 8 != 6) {
        assembler->nop();
      }
      offset_ = assembler->size() + kMovImmOffset;
      assert(offset_ % 8 == 0);
      assembler->mov(reg, dummy);
    }

    void Repatch(Assembler* assembler, uint64_t data) const {
      assembler->rewrite(offset_, data, k64Size);
    }

    std::size_t offset() const { return offset_; }

    static void RepatchAfterCall(void** ret, uint64_t ptr) {
      *reinterpret_cast<void**>((reinterpret_cast<uint8_t*>(*ret) - kRepatchOffset)) = reinterpret_cast<void*>(ptr);
    }

   private:
    std::size_t offset_;
  };

  Assembler()
    : core::XbyakMMapAllocator()
    , Xbyak::CodeGenerator(
        4096,
        Xbyak::AutoGrow,
        static_cast<core::XbyakMMapAllocator*>(this)) { }

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

  template<typename Func>
  void Call(Func* f) {
    mov(rax, core::BitCast<uintptr_t>(f));
    call(rax);
  }

  std::size_t size() const { return getSize(); }

  void* GainExecutableByOffset(std::size_t offset) const {
    return core::BitCast<void*>(getCode() + offset);
  }
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_ASSEMBLER_H_
