#ifndef IV_LV5_BREAKER_ASSEMBLER_H_
#define IV_LV5_BREAKER_ASSEMBLER_H_
#include <iv/detail/memory.h>
#include <iv/noncopyable.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/ic.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Assembler : public Xbyak::CodeGenerator {
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

  typedef std::pair<std::size_t, std::size_t> PCOffsetAndBytecodeOffset;
  typedef core::SortedVector<PCOffsetAndBytecodeOffset> BytecodeOffsets;
  typedef std::vector<std::shared_ptr<IC> > ICVector;

  Assembler()
    : Xbyak::CodeGenerator(4096, Xbyak::AutoGrow),
      bytecode_offsets_() {
    bytecode_offsets_.reserve(1024);
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

  struct Comparator {
    bool operator()(const PCOffsetAndBytecodeOffset& offset,
                    std::size_t pc_offset) const {
      return offset.first < pc_offset;
    }

    bool operator()(std::size_t pc_offset,
                    const PCOffsetAndBytecodeOffset& offset) const {
      return pc_offset < offset.first;
    }
  };

  void AttachBytecodeOffset(std::size_t pc_offset,
                            std::size_t bytecode_offset) {
    if (bytecode_offsets_.empty()) {
      bytecode_offsets_.push_back(std::make_pair(pc_offset, bytecode_offset));
      return;
    }

    PCOffsetAndBytecodeOffset& offset = bytecode_offsets_.back();
    if (offset.first == pc_offset) {
      offset.second = bytecode_offset;
      return;
    }

    if (offset.second != bytecode_offset) {
      bytecode_offsets_.push_back(std::make_pair(pc_offset, bytecode_offset));
    }
  }

  std::size_t PCToBytecodeOffset(void* pc) const {
    const BytecodeOffsets::const_iterator it =
        std::upper_bound(
            bytecode_offsets_.begin(),
            bytecode_offsets_.end(),
            core::BitCast<uint64_t>(pc) - core::BitCast<uint64_t>(getCode()),
            Comparator());
    if (it != bytecode_offsets_.begin()) {
      return (it - 1)->second;
    }
    return 0;
  }

  void BindIC(std::shared_ptr<IC> ic) {
    ics_.push_back(ic);
  }

  std::size_t size() const { return getSize(); }

  void MarkChildren(radio::Core* core) {
    for (ICVector::const_iterator it = ics_.begin(),
         last = ics_.end(); it != last; ++it) {
      (*it)->MarkChildren(core);
    }
  }

  GC_ms_entry* MarkChildren(GC_word* top,
                            GC_ms_entry* entry,
                            GC_ms_entry* mark_sp_limit,
                            GC_word env) {
    for (ICVector::const_iterator it = ics_.begin(),
         last = ics_.end(); it != last; ++it) {
      entry = (*it)->MarkChildren(top, entry, mark_sp_limit, env);
    }
    return entry;
  }

 private:
  BytecodeOffsets bytecode_offsets_;
  ICVector ics_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_ASSEMBLER_H_
