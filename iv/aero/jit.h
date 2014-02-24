// AeroHand RegExp Runtime JIT Compiler
// only works on x64 and gcc or clang calling convensions
//
// special thanks
//   MITSUNARI Shigeo
//
#ifndef IV_AERO_JIT_H_
#define IV_AERO_JIT_H_
#include <vector>
#include <iv/detail/cstdint.h>
#include <iv/detail/type_traits.h>
#include <iv/platform.h>
#include <iv/conversions.h>
#include <iv/assoc_vector.h>
#include <iv/utils.h>
#include <iv/noncopyable.h>
#include <iv/aero/op.h>
#include <iv/aero/code.h>
#include <iv/aero/utility.h>
#include <iv/aero/jit_fwd.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/xbyak/xbyak.h>

namespace iv {
namespace aero {
namespace jit_detail {

static const char* kQuickCheckNextLabel = "QUICK_CHECK_NEXT";
static const char* kBackTrackLabel = "BACKTRACK";
static const char* kReturnLabel = "RETURN";
static const char* kFailureLabel = "FAILURE";
static const char* kErrorLabel = "ERROR";
static const char* kSuccessLabel = "SUCCESS";
static const char* kStartLabel = "START";

template<typename CharT = char>
struct Reg {
  typedef Xbyak::Reg8 type;
  static const type& GetR10(Xbyak::CodeGenerator* gen) {
    return gen->r10b;
  }
  static const type& GetR11(Xbyak::CodeGenerator* gen) {
    return gen->r11b;
  }
};

template<>
struct Reg<char16_t> {
  typedef Xbyak::Reg16 type;
  static const type& GetR10(Xbyak::CodeGenerator* gen) {
    return gen->r10w;
  }
  static const type& GetR11(Xbyak::CodeGenerator* gen) {
    return gen->r11w;
  }
};

}  // namespace jit_detail

template<typename CharT>
class JIT : public Xbyak::CodeGenerator {
 public:
  // x64 JIT
  //
  // r10 : tmp
  // r11 : tmp
  //
  // callee-save
  // r12 : const char* subject
  // r13 : uint64_t size
  // r14 : int* captures
  // r15 : current position
  // rbx : stack position

  typedef std::unordered_map<int, uintptr_t> BackTrackMap;
  typedef typename jit_detail::Reg<CharT>::type RegC;
  typedef typename JITExecutable<CharT>::Executable Executable;
  typedef typename Code::Data::const_pointer const_pointer;

  class LocalLabel : private core::Noncopyable<> {
   public:
    explicit LocalLabel(JIT* gen)
      : gen_(gen) {
      gen_->inLocalLabel();
    }
    ~LocalLabel() {
      gen_->outLocalLabel();
    }
   private:
    JIT* gen_;
  };

  static bool IsAvailableSSE42() {
    static const bool result = mie::isAvailableSSE42();
    return result;
  }

  #define IV_AERO_LOCAL()\
    for (bool step = true; step;) \
      for (LocalLabel label(this); step; step = false)

  static_assert(
      std::is_same<CharT, char>::value || std::is_same<CharT, char16_t>::value,
      "CharT shoud be either char or char16_t");

  static const int kIntSize = core::Size::kIntSize;
  static const int kPtrSize = core::Size::kPointerSize;
  static const int kCharSize = sizeof(CharT);  // NOLINT
  static const int k8Size = sizeof(uint8_t);  // NOLINT
  static const int k16Size = sizeof(uint16_t);  // NOLINT
  static const int k32Size = sizeof(uint32_t);  // NOLINT
  static const int k64Size = sizeof(uint64_t);  // NOLINT
  static const int kStackSize = k64Size * 11;

  static const int kASCII = kCharSize == 1;

  explicit JIT(const Code& code)
    : Xbyak::CodeGenerator(4096, Xbyak::AutoGrow),
      code_(code),
      first_instr_(code.bytes().data()),
      targets_(),
      backtracks_(),
      tracked_(),
      character(kASCII ? byte : word),
      subject_(r12),
      size_(r13),
      captures_(r14),
      cp_(r15),
      cpd_(r15d),
      sp_(rbx),
      spd_(ebx),
      ch10_(jit_detail::Reg<CharT>::GetR10(this)),
      ch11_(jit_detail::Reg<CharT>::GetR11(this)) {
  }

  Executable Compile() {
    // start
    ScanJumpReference();
    EmitPrologue();
    Main();
    EmitEpilogue();
    return Get();
  }

  Executable Get() {
    return core::BitCast<Executable>(getCode());
  }

 private:
  void LoadVM(const Xbyak::Reg64& dst) {
    mov(dst, ptr[rsp + OFFSET_VM * k64Size]);
  }

  void LoadResult(const Xbyak::Reg64& dst) {
    mov(dst, ptr[rsp + OFFSET_CAPTURE * k64Size]);
  }

  void LoadStack(const Xbyak::Reg64& dst) {
    LoadVM(dst);
    mov(dst, ptr[dst]);
  }

  void LoadStateSize(const Xbyak::Reg64& dst) {
    LoadVM(dst);
    mov(dst, qword[dst + kPtrSize * 2 + k64Size]);
  }

  static int StaticPrint(const char* format, int64_t ch) {
    printf("%s: %lld\n", format, ch);
    return 0;
  }

  // debug
  template<typename REG>
  void Put(const char* format, const REG& reg) {
    push(r10);
    push(r11);
    push(rdi);
    push(rsi);
    push(rax);
    push(rdx);
    push(rcx);
    push(r8);

    mov(rdi, core::BitCast<uintptr_t>(format));
    mov(rsi, reg);
    mov(rax, core::BitCast<uintptr_t>(&StaticPrint));
    call(rax);

    pop(r8);
    pop(rcx);
    pop(rdx);
    pop(rax);
    pop(rsi);
    pop(rdi);
    pop(r11);
    pop(r10);
  }

  void ScanJumpReference() {
    // scan jump referenced opcode, and record it to jump table
    const_pointer instr = first_instr_;
    const const_pointer last = code_.bytes().data() + code_.bytes().size();
    while (instr != last) {
      const uint8_t opcode = *instr;
      const uint32_t length = OP::GetLength(instr);
      switch (opcode) {
        case OP::PUSH_BACKTRACK: {
          const int dis = Load4Bytes(instr + 1);
          if (backtracks_.find(dis) == backtracks_.end()) {
            backtracks_.insert(std::make_pair(dis, backtracks_.size()));
          }
          break;
        }
        case OP::COUNTER_NEXT: {
          targets_.insert(Load4Bytes(instr + 9));
          break;
        }
        case OP::ASSERTION_SUCCESS: {
          targets_.insert(Load4Bytes(instr + 5));
          break;
        }
        case OP::JUMP: {
          targets_.insert(Load4Bytes(instr + 1));
          break;
        }
      }
      std::advance(instr, length);
    }
    tracked_.resize(backtracks_.size(), 0);
  }

  static bool CanOptimize(uint8_t opcode) {
    return OP::STORE_SP <= opcode;
  }

  bool ReferencedByJump(const_pointer instr) const {
    const uint32_t offset = instr - first_instr_;
    if (std::binary_search(targets_.begin(), targets_.end(), offset)) {
      return true;
    }
    if (backtracks_.find(offset) != backtracks_.end()) {
      return true;
    }
    return false;
  }

  static uint32_t GuardedSize(uint8_t opcode) {
    assert(CanOptimize(opcode));
    return (opcode > OP::COUNTER_ZERO) ? 1 : 0;
  }

  bool EmitOptimized(const_pointer instr,
                     const_pointer last,
                     const_pointer* result) {
    // first
    assert(CanOptimize(*instr));
    assert(instr != last);

    uint32_t guarded = JIT::GuardedSize(*instr);
    const_pointer current = instr;
    std::advance(current, OP::GetLength(current));
    const const_pointer start = current;

    // calculate size
    while (current != last) {
      if (CanOptimize(*current) && !ReferencedByJump(current)) {
        // ok
        guarded += JIT::GuardedSize(*current);
      } else {
        // bailout
        break;
      }
      std::advance(current, OP::GetLength(current));
    }

    if (current == start) {
      // not optimized
      return false;
    }

    lea(rbp, ptr[subject_ + cp_ * kCharSize]);
    if (guarded > 0) {
      add(cp_, guarded);
      cmp(cp_, size_);
      jg(jit_detail::kBackTrackLabel, T_NEAR);
    }
    int offset = 0;
    while (instr != current) {
      const uint32_t length = OP::GetLength(instr);
      switch (*instr) {
        case OP::CHECK_1BYTE_CHAR:
          EmitCHECK_1BYTE_CHAR(instr, length, offset);
          break;
        case OP::CHECK_2BYTE_CHAR:
          EmitCHECK_2BYTE_CHAR(instr, length, offset);
          break;
        case OP::CHECK_2CHAR_OR:
          EmitCHECK_2CHAR_OR(instr, length, offset);
          break;
        case OP::CHECK_3CHAR_OR:
          EmitCHECK_3CHAR_OR(instr, length, offset);
          break;
        case OP::CHECK_4CHAR_OR:
          EmitCHECK_4CHAR_OR(instr, length, offset);
          break;
        case OP::CHECK_RANGE:
          EmitCHECK_RANGE(instr, length, offset);
          break;
        case OP::CHECK_RANGE_INVERTED:
          EmitCHECK_RANGE_INVERTED(instr, length, offset);
          break;
        case OP::STORE_SP:
          EmitSTORE_SP(instr, length);
          break;
        case OP::COUNTER_ZERO:
          EmitCOUNTER_ZERO(instr, length);
          break;
        default: {
          UNREACHABLE();
        }
      }
      offset += JIT::GuardedSize(*instr);
      std::advance(instr, length);
    }
    *result = current;
    return true;
  }

  void Main() {
    // main pass
    const_pointer instr = first_instr_;
    const const_pointer last = code_.bytes().data() + code_.bytes().size();

    while (instr != last) {
      const uint8_t opcode = *instr;
      const uint32_t length = OP::GetLength(instr);
      const uint32_t offset = instr - first_instr_;
      if (std::binary_search(targets_.begin(), targets_.end(), offset)) {
        DefineLabel(offset);
      }
      const BackTrackMap::const_iterator it = backtracks_.find(offset);
      if (it != backtracks_.end()) {
          tracked_[it->second] = core::BitCast<uintptr_t>(getSize()); // offset from getCode
      }

      // basic block optimization pass
      if (JIT::CanOptimize(opcode) && EmitOptimized(instr, last, &instr)) {
        continue;
      }

#define INTERCEPT()\
  do {\
    mov(r10, offset);\
    Put("OPCODE", r10);\
  } while (0)
      // INTERCEPT();
#undef INTERCEPT

#define V(op, N)\
  case OP::op: {\
    Emit##op(instr, length);\
    break;\
  }
      switch (opcode) {
IV_AERO_OPCODES(V)
      }
#undef V
      std::advance(instr, length);
    }
  }

  void EmitQuickCheck() {
    const uint16_t filter = code_.filter();
    inLocalLabel();
    if (!filter) {
      // normal path
      L(".QUICK_CHECK_NORMAL_START");
      jmp(jit_detail::kStartLabel, T_NEAR);
      L(jit_detail::kQuickCheckNextLabel);
      inc(cp_);
      cmp(cp_, size_);
      jle(".QUICK_CHECK_NORMAL_START");
      jmp(jit_detail::kFailureLabel, T_NEAR);
    } else {
      // one char check and bloom filter path
      if (code_.IsQuickCheckOneChar() && kASCII && !core::character::IsASCII(filter)) {
        jmp(jit_detail::kFailureLabel, T_NEAR);
        L(jit_detail::kQuickCheckNextLabel);
        return;
      }
      L(".QUICK_CHECK_SPECIAL_START");
      cmp(cp_, size_);
      jge(jit_detail::kFailureLabel, T_NEAR);
      movzx(r10, character[subject_ + cp_ * kCharSize]);
      if (code_.IsQuickCheckOneChar()) {
        cmp(r10, filter);
      } else {
        mov(r11, r10);
        and(r10, filter);
        cmp(r11, r10);
      }
      jne(jit_detail::kQuickCheckNextLabel, T_NEAR);
      jmp(jit_detail::kStartLabel, T_NEAR);
      L(jit_detail::kQuickCheckNextLabel);
      inc(cp_);
      jmp(".QUICK_CHECK_SPECIAL_START");
    }
    outLocalLabel();
  }

  enum {
    OFFSET_VM = 0,
    OFFSET_CAPTURE,
    OFFSET_CP
  };

  void EmitPrologue() {
    // initialize capture buffer and put arguments registers
    sub(rsp, kStackSize);
    mov(qword[rsp + k64Size * OFFSET_VM], rdi);  // push vm to stack
    mov(qword[rsp + k64Size * OFFSET_CAPTURE], rcx);  // push captures to stack
    // 8 * 3 is reserved for cp store
    mov(qword[rsp + k64Size * 4], subject_);
    mov(qword[rsp + k64Size * 5], captures_);
    mov(qword[rsp + k64Size * 6], size_);
    mov(qword[rsp + k64Size * 7], cp_);
    mov(qword[rsp + k64Size * 8], sp_);
    mov(qword[rsp + k64Size * 9], rbp);

    // calling convension is
    // Execute(VM* vm, const char* subject,
    //         uint32_t size, int* captures, uint32_t cp)
    mov(subject_, rsi);
    mov(size_, rdx);
    mov(cp_, r8);

    const std::size_t size = code_.captures() * 2 + code_.counters() + 1;
    {
      // allocate state space
      // rdi is already VM*
      inLocalLabel();
      mov(captures_, ptr[rdi + kPtrSize]);  // load state
      cmp(qword[rdi + kPtrSize * 2 + k64Size], size);
      jge(".EXIT", T_NEAR);
      xchg(rdi, captures_);
      mov(rax, core::BitCast<uintptr_t>(&std::free));
      call(rax);
      // state size is too small
      const std::size_t allocated = IV_ROUNDUP(size, VM::kInitialStateSize);
      mov(rdi, allocated * kIntSize);
      mov(rax, core::BitCast<uintptr_t>(&std::malloc));
      call(rax);
      mov(qword[captures_ + kPtrSize], rax);
      mov(qword[captures_ + kPtrSize * 2 + k64Size], allocated);
      mov(captures_, rax);
      L(".EXIT");
      outLocalLabel();
    }

    // generate quick check path
    EmitQuickCheck();

    L(jit_detail::kStartLabel);
    {
      mov(qword[rsp + k64Size * OFFSET_CP], cp_);
      // initialize sp_ to 0
      xor(sp_, sp_);

      // initialize captures
      if ((size - 1) != 0) {
        IV_AERO_LOCAL() {
          mov(r11d, size - 1);
          L(".LOOP_START");
          mov(dword[captures_ + r11 * kIntSize], kUndefined);
          sub(r11d, 1);
          jnz(".LOOP_START");
          L(".LOOP_END");
        }
      }
      mov(dword[captures_], cpd_);
    }
  }

  void EmitEpilogue() {
    // generate success path
    L(jit_detail::kSuccessLabel);
    {
      inLocalLabel();
      mov(dword[captures_ + kIntSize], cpd_);
      // copy to result
      LoadResult(r10);
      mov(r11, code_.captures() * 2);
      mov(rax, captures_);

      test(r11, r11);
      jz(".LOOP_END");

      L(".LOOP_START");
      mov(ecx, dword[rax]);
      add(rax, kIntSize);
      mov(dword[r10], ecx);
      add(r10, kIntSize);
      sub(r11, 1);
      jnz(".LOOP_START");
      L(".LOOP_END");
      mov(rax, AERO_SUCCESS);
      outLocalLabel();
      // fall through
    }

    // generate last return path
    L(jit_detail::kReturnLabel);
    {
      mov(subject_, qword[rsp + k64Size * 4]);
      mov(captures_, qword[rsp + k64Size * 5]);
      mov(size_, qword[rsp + k64Size * 6]);
      mov(cp_, qword[rsp + k64Size * 7]);
      mov(sp_, qword[rsp + k64Size * 8]);
      mov(rbp, qword[rsp + k64Size * 9]);
      add(rsp, kStackSize);
      ret();
      align(16);
    }

    // generate last failure path
    L(jit_detail::kFailureLabel);
    {
      mov(rax, AERO_FAILURE);
      jmp(jit_detail::kReturnLabel);
    }

    // generate backtrack path
    {
      inLocalLabel();
      L(jit_detail::kBackTrackLabel);
      test(sp_, sp_);
      jz(".FAILURE", T_NEAR);
      const int size = code_.captures() * 2 + code_.counters() + 1;
      sub(sp_, size);

      // copy to captures
      mov(r10, captures_);
      mov(r11, size);
      LoadStack(rax);
      lea(rax, ptr[rax + sp_ * kIntSize]);

      test(r11, r11);
      jz(".LOOP_END");

      L(".LOOP_START");
      mov(ecx, dword[rax]);
      add(rax, kIntSize);
      mov(dword[r10], ecx);
      add(r10, kIntSize);
      sub(r11, 1);
      jnz(".LOOP_START");
      L(".LOOP_END");

      movsxd(cp_, dword[captures_ + kIntSize]);
      movsxd(rcx, dword[captures_ + kIntSize * (size - 1)]);
      mov(rax, core::BitCast<uintptr_t>(tracked_.data()));
      jmp(ptr[rax + rcx * kPtrSize]);

      L(".FAILURE");
      mov(cp_, qword[rsp + k64Size * OFFSET_CP]);
      jmp(jit_detail::kQuickCheckNextLabel, T_NEAR);
      outLocalLabel();
    }

    // generate error path
    L(jit_detail::kErrorLabel);
    {
      mov(rax, AERO_ERROR);
      jmp(jit_detail::kReturnLabel);
    }
    // finish generating code and determine code address
    ready();
    {
      const uintptr_t top = core::BitCast<uintptr_t>(getCode());
      for (size_t i = 0; i < tracked_.size(); i++) {
        tracked_[i] += top;
      }
    }
  }

  void EmitSTORE_SP(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(dword[captures_ + kIntSize * offset], spd_);
  }

  void EmitSTORE_POSITION(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(dword[captures_ + kIntSize * offset], cpd_);
  }

  void EmitPOSITION_TEST(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    cmp(cpd_, dword[captures_ + kIntSize * offset]);
    je(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitASSERTION_SUCCESS(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    movsxd(sp_, dword[captures_ + kIntSize * offset]);
    LoadStack(r10);
    movsxd(cp_, dword[r10 + sp_ * kIntSize + kIntSize]);
    Jump(Load4Bytes(instr + 5));
  }

  void EmitASSERTION_FAILURE(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    movsxd(sp_, dword[captures_ + kIntSize * offset]);
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void InlineIsLineTerminator(const RegC& reg, const char* ok) {
    cmp(reg, core::character::code::CR);
    je(ok);
    cmp(reg, core::character::code::LF);
    je(ok);
    if (!kASCII) {
      // not ASCII => 16bit
      // (c & ~1) == 0x2028;  // 0x2028 or 0x2029
      const Xbyak::Reg32 reg32(reg.getIdx());
      and(reg32, 0xFFFD);
      cmp(reg32, 0x2028);
      je(ok);
    }
  }

  void EmitASSERTION_BOL(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    test(cp_, cp_);
    jz(".SUCCESS");
    mov(ch10_, character[subject_ + (cp_ * kCharSize) - kCharSize]);
    InlineIsLineTerminator(ch10_, ".SUCCESS");
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    outLocalLabel();
  }

  void EmitASSERTION_BOB(const uint8_t* instr, uint32_t len) {
    test(cp_, cp_);
    jnz(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitASSERTION_EOL(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    cmp(cp_, size_);
    je(".SUCCESS");
    mov(ch10_, character[subject_ + (cp_ * kCharSize)]);
    InlineIsLineTerminator(ch10_, ".SUCCESS");
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    outLocalLabel();
  }

  void EmitASSERTION_EOB(const uint8_t* instr, uint32_t len) {
    cmp(cp_, size_);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void SetRegIfIsWord(const Xbyak::Reg64& out, const Xbyak::Reg64& reg) {
    inLocalLabel();
    if (!kASCII) {
      // insert ASCII check
      test(reg, static_cast<uint16_t>(65408));  // (1111111110000000)2
      jnz(".exit");
    }
    static const char tbl[128] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
    };
    xor(out, out);
    mov(out, reinterpret_cast<uintptr_t>(tbl));  // get tbl address
    movzx(out, byte[out + reg]);
    L(".exit");
    outLocalLabel();
  }

  void EmitASSERTION_WORD_BOUNDARY(const uint8_t* instr, uint32_t len) {
    inLocalLabel();

    test(cpd_, cpd_);
    jz(".FIRST_FALSE");
    cmp(cp_, size_);
    jg(".FIRST_FALSE");
    movzx(r10, character[subject_ + (cp_ * kCharSize) - kCharSize]);
    SetRegIfIsWord(rax, r10);
    jmp(".SECOND");
    L(".FIRST_FALSE");
    xor(rax, rax);

    L(".SECOND");
    cmp(cp_, size_);
    jge(".SECOND_FALSE");
    movzx(r11, character[subject_ + cp_ * kCharSize]);
    SetRegIfIsWord(rcx, r11);
    jmp(".CHECK");
    L(".SECOND_FALSE");
    xor(rcx, rcx);

    L(".CHECK");
    xor(rax, rcx);
    jz(jit_detail::kBackTrackLabel, T_NEAR);
    outLocalLabel();
  }

  void EmitASSERTION_WORD_BOUNDARY_INVERTED(const uint8_t* instr, uint32_t len) {  // NOLINT
    inLocalLabel();

    test(cpd_, cpd_);
    jz(".FIRST_FALSE");
    cmp(cp_, size_);
    jg(".FIRST_FALSE");
    movzx(r10, character[subject_ + (cp_ * kCharSize) - kCharSize]);
    SetRegIfIsWord(rax, r10);
    jmp(".SECOND");
    L(".FIRST_FALSE");
    xor(rax, rax);

    L(".SECOND");
    cmp(cp_, size_);
    jge(".SECOND_FALSE");
    movzx(r11, character[subject_ + cp_ * kCharSize]);
    SetRegIfIsWord(rcx, r11);
    jmp(".CHECK");
    L(".SECOND_FALSE");
    xor(rcx, rcx);

    L(".CHECK");
    xor(rax, rcx);
    jnz(jit_detail::kBackTrackLabel, T_NEAR);
    outLocalLabel();
  }

  void EmitSTART_CAPTURE(const uint8_t* instr, uint32_t len) {
    const uint32_t target = Load4Bytes(instr + 1);
    mov(dword[captures_ + kIntSize * (target * 2)], cpd_);
    mov(dword[captures_ + kIntSize * (target * 2 + 1)], kUndefined);
  }

  void EmitEND_CAPTURE(const uint8_t* instr, uint32_t len) {
    const uint32_t target = Load4Bytes(instr + 1);
    mov(dword[captures_ + kIntSize * (target * 2 + 1)], cpd_);
  }

  void EmitCLEAR_CAPTURES(const uint8_t* instr, uint32_t len) {
    const uint32_t from = Load4Bytes(instr + 1) * 2;
    const uint32_t to = Load4Bytes(instr + 5) * 2;
    if ((to - from) <= 10) {
      // loop unrolling
      if ((to - from) > 0) {
        lea(r10, ptr[captures_ + kIntSize * from]);
        uint32_t i = from;
        while (true) {
          mov(dword[r10], kUndefined);
          ++i;
          if (i == to) {
            break;
          }
          add(r10, kIntSize);
        }
      }
    } else {
      assert(to - from);  // not 0
      inLocalLabel();
      mov(r10, (to - from));
      lea(r11, ptr[captures_ + kIntSize * from]);
      L(".LOOP_START");
      mov(dword[r11], kUndefined);
      add(r11, kIntSize);
      sub(r10, 1);
      jnz(".LOOP_START");
      outLocalLabel();
    }
  }

  void EmitCOUNTER_ZERO(const uint8_t* instr, uint32_t len) {
    const uint32_t counter = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(dword[captures_ + kIntSize * counter], 0);
  }

  void EmitCOUNTER_NEXT(const uint8_t* instr, uint32_t len) {
    const int max = static_cast<int>(Load4Bytes(instr + 5));
    const uint32_t counter = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(r10d, dword[captures_ + kIntSize * counter]);
    inc(r10d);
    mov(dword[captures_ + kIntSize * counter], r10d);
    cmp(r10d, max);
    jl(JIT::MakeLabel(Load4Bytes(instr + 9)).c_str(), T_NEAR);
  }

  void EmitPUSH_BACKTRACK(const uint8_t* instr, uint32_t len) {
    const int size = code_.captures() * 2 + code_.counters() + 1;
    inLocalLabel();
    LoadVM(rdi);
    add(sp_, size);
    mov(rcx, ptr[rdi + kPtrSize * 2]);  // stack size
    mov(rsi, sp_);  // offset
    cmp(rsi, rcx);
    jle(".ALLOCATABLE");

    mov(rax, core::BitCast<uintptr_t>(&VM::NewStateForJIT));
    call(rax);
    test(rax, rax);
    jz(jit_detail::kErrorLabel, T_NEAR);

    L(".ALLOCATABLE");
    LoadVM(rdi);
    mov(rdi, ptr[rdi]);
    lea(rsi, ptr[rdi + (sp_ * kIntSize) - (kIntSize * size)]);

    // copy
    if ((size - 1) != 0) {
      mov(r11d, size - 1);
      L(".LOOP_START");
      sub(r11d, 1);
      mov(ecx, dword[captures_ + r11 * kIntSize]);
      mov(dword[rsi + r11 * kIntSize], ecx);
      jnz(".LOOP_START");
      L(".LOOP_END");
    }

    mov(dword[rsi + kIntSize], cpd_);
    const int val = static_cast<int>(Load4Bytes(instr + 1));
    const BackTrackMap::const_iterator it = backtracks_.find(val);
    assert(it != backtracks_.end());
    // we use rsi as counter in AERO_PUSH_BACKTRACK
    mov(dword[rsi + kIntSize * (size - 1)], static_cast<uint32_t>(it->second));
    outLocalLabel();
  }

  void EmitBACK_REFERENCE(const uint8_t* instr, uint32_t len) {
    const uint16_t ref = Load2Bytes(instr + 1);
    assert(ref != 0);  // limited by parser
    if (ref >= code_.captures()) {
      return;
    }

    inLocalLabel();
    movsxd(rax, dword[captures_ + kIntSize * (ref * 2 + 1)]);
    cmp(rax, -1);
    je(".SUCCESS", T_NEAR);
    movsxd(r10, dword[captures_ + kIntSize * (ref * 2)]);
    sub(rax, r10);
    lea(rcx, ptr[rax + cp_]);

    cmp(rcx, size_);
    jg(jit_detail::kBackTrackLabel, T_NEAR);

    // back reference check
    lea(rdx, ptr[subject_ + r10 * kCharSize]);
    lea(rcx, ptr[subject_ + cp_ * kCharSize]);
    mov(r8, rax);

    test(r8, r8);
    jz(".LOOP_END");

    L(".LOOP_START");
    mov(ch10_, character[rdx]);
    cmp(ch10_, character[rcx]);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
    add(rdx, kCharSize);
    add(rcx, kCharSize);
    sub(r8, 1);
    jnz(".LOOP_START");
    L(".LOOP_END");

    add(cp_, rax);

    L(".SUCCESS");
    outLocalLabel();
  }

  void EmitBACK_REFERENCE_IGNORE_CASE(const uint8_t* instr, uint32_t len) {
    const uint16_t ref = Load2Bytes(instr + 1);
    assert(ref != 0);  // limited by parser
    if (ref >= code_.captures()) {
      return;
    }

    inLocalLabel();
    movsxd(rax, dword[captures_ + kIntSize * (ref * 2 + 1)]);
    cmp(rax, -1);
    je(".SUCCESS", T_NEAR);
    movsxd(r10, dword[captures_ + kIntSize * (ref * 2)]);
    sub(rax, r10);
    lea(rcx, ptr[rax + cp_]);

    cmp(rcx, size_);
    jg(jit_detail::kBackTrackLabel, T_NEAR);

    // back reference check
    lea(rdx, ptr[subject_ + r10 * kCharSize]);
    lea(rcx, ptr[subject_ + cp_ * kCharSize]);
    mov(r8, rax);

    test(r8, r8);
    jz(".LOOP_END");

    L(".LOOP_START");
    mov(ch10_, character[rdx]);
    cmp(ch10_, character[rcx]);
    je(".COND_OK", T_NEAR);

    // used callar-save registers
    // r8 r10 r11 rdx rcx
    push(r8);
    push(r8);

    push(rdx);
    push(rcx);
    movzx(rdi, ch10_);
    mov(r10, core::BitCast<uintptr_t>(&core::character::ToUpperCase));
    call(r10);
    pop(rcx);
    cmp(eax, character[rcx]);
    je(".CALL_COND_OK");

    pop(rdx);
    movzx(rdi, character[rdx]);
    push(rdx);
    push(rcx);
    movzx(rdi, ch10_);
    mov(r10, core::BitCast<uintptr_t>(&core::character::ToLowerCase));
    call(r10);
    pop(rcx);
    cmp(eax, character[rcx]);
    je(".CALL_COND_OK");

    pop(rdx);
    pop(r8);
    pop(r8);
    jmp(jit_detail::kBackTrackLabel, T_NEAR);

    L(".CALL_COND_OK");
    pop(rdx);
    pop(r8);
    pop(r8);

    L(".COND_OK");
    add(rdx, kCharSize);
    add(rcx, kCharSize);
    sub(r8, 1);
    jnz(".LOOP_START");

    L(".LOOP_END");
    add(cp_, rax);

    L(".SUCCESS");
    outLocalLabel();
  }

  void EmitCHECK_1BYTE_CHAR(const uint8_t* instr, uint32_t len, int offset = -1) {
    const CharT ch = Load1Bytes(instr + 1);
    EmitSizeGuard(offset);
    cmp(LoadCode(offset), ch);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
    IncrementCP(offset);
  }

  void EmitCHECK_2BYTE_CHAR(const uint8_t* instr, uint32_t len, int offset = -1) {
    if (kASCII) {
      jmp(jit_detail::kBackTrackLabel, T_NEAR);
      return;
    }
    const char16_t ch = Load2Bytes(instr + 1);
    EmitSizeGuard(offset);
    if (ch <= 0x7FFF) {  // INT16_MAX
      cmp(LoadCode(offset), ch);
    } else {
      movzx(r10, LoadCode(offset));
      cmp(r10, ch);
    }
    jne(jit_detail::kBackTrackLabel, T_NEAR);
    IncrementCP(offset);
  }

  void EmitCHECK_2CHAR_OR(const uint8_t* instr, uint32_t len, int offset = -1) {
    const char16_t first = Load2Bytes(instr + 1);
    const char16_t second = Load2Bytes(instr + 3);
    inLocalLabel();
    EmitSizeGuard(offset);
    movzx(r10, LoadCode(offset));
    if (!(kASCII && !core::character::IsASCII(first))) {
      cmp(r10, first);
      je(".SUCCESS");
    }
    if (!(kASCII && !core::character::IsASCII(second))) {
      cmp(r10, second);
      je(".SUCCESS");
    }
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    IncrementCP(offset);
    outLocalLabel();
  }

  void EmitCHECK_3CHAR_OR(const uint8_t* instr, uint32_t len, int offset = -1) {
    const char16_t first = Load2Bytes(instr + 1);
    const char16_t second = Load2Bytes(instr + 3);
    const char16_t third = Load2Bytes(instr + 5);
    inLocalLabel();
    EmitSizeGuard(offset);
    movzx(r10, LoadCode(offset));
    if (!(kASCII && !core::character::IsASCII(first))) {
      cmp(r10, first);
      je(".SUCCESS");
    }
    if (!(kASCII && !core::character::IsASCII(second))) {
      cmp(r10, second);
      je(".SUCCESS");
    }
    if (!(kASCII && !core::character::IsASCII(third))) {
      cmp(r10, third);
      je(".SUCCESS");
    }
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    IncrementCP(offset);
    outLocalLabel();
  }

  void EmitCHECK_4CHAR_OR(const uint8_t* instr, uint32_t len, int offset = -1) {
    const char16_t first = Load2Bytes(instr + 1);
    const char16_t second = Load2Bytes(instr + 3);
    const char16_t third = Load2Bytes(instr + 5);
    const char16_t fourth = Load2Bytes(instr + 7);
    inLocalLabel();
    EmitSizeGuard(offset);
    movzx(r10, LoadCode(offset));
    if (!(kASCII && !core::character::IsASCII(first))) {
      cmp(r10, first);
      je(".SUCCESS");
    }
    if (!(kASCII && !core::character::IsASCII(second))) {
      cmp(r10, second);
      je(".SUCCESS");
    }
    if (!(kASCII && !core::character::IsASCII(third))) {
      cmp(r10, third);
      je(".SUCCESS");
    }
    if (!(kASCII && !core::character::IsASCII(fourth))) {
      cmp(r10, fourth);
      je(".SUCCESS");
    }
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    IncrementCP(offset);
    outLocalLabel();
  }

  void EmitCHECK_RANGE(const uint8_t* instr, uint32_t len, int offset = -1) {
    const uint32_t length = Load4Bytes(instr + 1);
    const uint32_t counts = Load4Bytes(instr + 5);

    // If counts is less than or equal 8 and SSE4.2 is enabled,
    // we create mask from candidate characters and apply pcmpestri.
    // TODO(Yusuke Suzuki):
    // When target character is ascii, we can compare 16 characters at once.
    if (IsAvailableSSE42() && counts <= 8) {
      union {
        std::array<uint16_t, 8> b16;
        std::array<uint64_t, 2> b64;
      } buffer{};
      auto it = buffer.b16.begin();
      for (std::size_t i = 0; i < length; i += 4) {
        const char16_t finish = Load2Bytes(instr + 5 + 4 + i + 2);
        for (char16_t cur = Load2Bytes(instr + 5 + 4 + i);
             cur <= finish; ++cur) {
          *it++ = cur;
        }
      }
      EmitSizeGuard(offset);
      movzx(eax, LoadCode(offset));
      movd(xm0, eax);

      mov(rax, buffer.b64[0]);
      pinsrq(xm1, rax, 0);
      if (counts > 4) {
        mov(rax, buffer.b64[1]);
        pinsrq(xm1, rax, 1);
      }

      mov(eax, 1);  // 1(u16)
      mov(edx, counts);  // counts(u16)
      pcmpestri(xm0, xm1, 0x1);
      jnc(jit_detail::kBackTrackLabel, T_NEAR);
      IncrementCP(offset);
      return;
    }

    const std::size_t ranges = length / 4;
    // Generate range check code with SSE4.2
    // TODO(Yusuke Suzuki):
    // When target character is ascii, we can compare 8 ranges at once.
    if (IsAvailableSSE42() && ranges >= 4) {
      IV_AERO_LOCAL() {
        EmitSizeGuard(offset);
        movzx(eax, LoadCode(offset));
        movd(xm1, eax);
        mov(edx, 1);  // 1(u16)
        const std::size_t ranges = length / 4;
        // We can check 4 ranges at once.
        for (std::size_t base = 0; base < ranges; base += 4) {
          union {
            std::array<uint16_t, 8> b16;
            std::array<uint64_t, 2> b64;
          } buffer{};
          const std::size_t counts =
              (std::min<std::size_t>)(4, (ranges - base));
          for (std::size_t i = 0; i < counts; ++i) {
            const std::size_t range = (i + base) * 4;
            const char16_t start = Load2Bytes(instr + 5 + 4 + range);
            const char16_t finish = Load2Bytes(instr + 5 + 4 + range + 2);
            buffer.b16[i * 2] = start;
            buffer.b16[i * 2 + 1] = finish;
          }
          mov(r10, buffer.b64[0]);
          pinsrq(xm0, r10, 0);
          if (counts > 2) {
            mov(r10, buffer.b64[1]);
            pinsrq(xm0, r10, 1);
          }
          mov(eax, (counts * 2));  // (pair * 2) (u16)
          pcmpestri(xm0, xm1, 0x1 + 0x4);
          jc(".SUCCESS", T_NEAR);
        }
        jmp(jit_detail::kBackTrackLabel, T_NEAR);
        L(".SUCCESS");
        IncrementCP(offset);
      }
      return;
    }

    // Normal path.
    IV_AERO_LOCAL() {
      EmitSizeGuard(offset);
      movzx(r10, LoadCode(offset));
      for (std::size_t i = 0; i < length; i += 4) {
        const char16_t start = Load2Bytes(instr + 5 + 4 + i);
        const char16_t finish = Load2Bytes(instr + 5 + 4 + i + 2);
        if (kASCII && (!core::character::IsASCII(start))) {
          jmp(jit_detail::kBackTrackLabel, T_NEAR);
          break;
        }
        cmp(r10, start);
        jl(jit_detail::kBackTrackLabel, T_NEAR);

        if (kASCII && (!core::character::IsASCII(finish))) {
          jmp(".SUCCESS", T_NEAR);
          break;
        }
        cmp(r10, finish);
        jle(".SUCCESS", T_NEAR);
      }
      jmp(jit_detail::kBackTrackLabel, T_NEAR);
      L(".SUCCESS");
      IncrementCP(offset);
    }
  }

  void EmitCHECK_RANGE_INVERTED(const uint8_t* instr, uint32_t len, int offset = -1) {
    inLocalLabel();
    EmitSizeGuard(offset);
    movzx(r10, LoadCode(offset));
    const uint32_t length = Load4Bytes(instr + 1);
    const uint32_t counts = Load4Bytes(instr + 5);
    for (std::size_t i = 0; i < length; i += 4) {
      const char16_t start = Load2Bytes(instr + 5 + 4 + i);
      const char16_t finish = Load2Bytes(instr + 5 + 4 + i + 2);
      if (kASCII && (!core::character::IsASCII(start))) {
        jmp(".SUCCESS", T_NEAR);
        break;
      }
      cmp(r10, start);
      jl(".SUCCESS", T_NEAR);

      if (kASCII && (!core::character::IsASCII(finish))) {
        jmp(jit_detail::kBackTrackLabel, T_NEAR);
        break;
      }
      cmp(r10, finish);
      jle(jit_detail::kBackTrackLabel, T_NEAR);
    }
    L(".SUCCESS");
    IncrementCP(offset);
    outLocalLabel();
  }

  void EmitJUMP(const uint8_t* instr, uint32_t len) {
    uint32_t offset = Load4Bytes(instr + 1);
    while (*(first_instr_ + offset) == OP::JUMP) {
      offset = Load4Bytes(first_instr_ + offset + 1);
    }
    switch (*(first_instr_ + offset)) {
      case OP::SUCCESS:
        jmp(jit_detail::kSuccessLabel, T_NEAR);
        return;
      case OP::FAILURE:
        jmp(jit_detail::kBackTrackLabel, T_NEAR);
        return;
    }
    assert(*(first_instr_ + offset) != OP::JUMP);
    Jump(offset);
  }

  void EmitFAILURE(const uint8_t* instr, uint32_t len) {
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitSUCCESS(const uint8_t* instr, uint32_t len) {
    jmp(jit_detail::kSuccessLabel, T_NEAR);
  }

  static std::string MakeLabel(uint32_t num, const std::string& prefix = "AERO_") {
    std::string str(prefix);
    core::UInt32ToString(num, std::back_inserter(str));
    return str;
  }

  void DefineLabel(uint32_t num) {
    L(JIT::MakeLabel(num).c_str());
  }

  void Jump(uint32_t num) {
    jmp(JIT::MakeLabel(num).c_str(), T_NEAR);
  }

  void IncrementCP(int offset) {
    if (offset < 0) {
      inc(cp_);
    }
  }

  Xbyak::Address LoadCode(int offset) {
    if (offset < 0) {
      return character[subject_ + cp_ * kCharSize];
    } else {
      return character[rbp + offset * kCharSize];
    }
  }

  void EmitSizeGuard(int offset) {
    if (offset < 0) {
      cmp(cp_, size_);
      jge(jit_detail::kBackTrackLabel, T_NEAR);
    }
  }

  const Code& code_;
  const const_pointer first_instr_;
  core::SortedVector<uint32_t> targets_;
  BackTrackMap backtracks_;
  std::vector<uintptr_t> tracked_;

  const Xbyak::AddressFrame character;
  const Xbyak::Reg64& subject_;
  const Xbyak::Reg64& size_;
  const Xbyak::Reg64& captures_;
  const Xbyak::Reg64& cp_;
  const Xbyak::Reg32& cpd_;
  const Xbyak::Reg64& sp_;
  const Xbyak::Reg32& spd_;
  const RegC& ch10_;
  const RegC& ch11_;
};

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_H_
