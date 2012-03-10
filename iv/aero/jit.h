#ifndef IV_AERO_JIT_H_
#define IV_AERO_JIT_H_
#include <vector>
#include <iv/detail/cstdint.h>
#include <iv/no_operator_names_guard.h>
#include <iv/conversions.h>
#include <iv/assoc_vector.h>
#include <iv/aero/op.h>
#include <iv/aero/code.h>
#include <iv/aero/utility.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/xbyak/xbyak.h>
namespace iv {
namespace aero {
namespace jit_detail {

static const char* kBackTrackLabel = "BACKTRACK";
static const char* kReturnLabel = "RETURN";

}  // namespace jit_detail

template<typename CharT>
class JIT : private Xbyak::CodeGenerator {
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

  typedef int(*Executable)(VM* vm, const CharT* subject, uint32_t size, int* captures, uint32_t cp);  // NOLINT
  typedef std::unordered_map<int, uintptr_t> BackTrackMap;

  static const int kVM = 0;

  explicit JIT(const Code& code)
    : code_(code),
      first_instr_(code.bytes().data()),
      targets_(),
      backtracks_(),
      tracked_(),
      character(sizeof(CharT) * CHAR_BIT),  // NOLINT
      subject_(r12),
      size_(r13),
      captures_(r14),
      cp_(r15),
      cpd_(r15d),
      sp_(rbx),
      spd_(ebx) { }

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
    mov(dst, ptr[rbp - 8]);
  }

  void LoadStack(const Xbyak::Reg64& dst) {
    LoadVM(dst);
    mov(dst, ptr[dst]);
  }

  static int StaticPrint(int ch) {
    printf("PUT: %d\n", ch);
    return 0;
  }

  void Put(const Xbyak::Reg64& reg) {
    mov(rdi, reg);
    mov(rax, core::BitCast<uintptr_t>(&StaticPrint));
    call(rax);
  }

  void ScanJumpReference() {
    // scan jump referenced opcode, and record it to jump table
    Code::Data::const_pointer instr = first_instr_;
    const Code::Data::const_pointer last =
        code_.bytes().data() + code_.bytes().size();
    while (instr != last) {
      const uint8_t opcode = *instr;
      uint32_t length = kOPLength[opcode];
      if (opcode == OP::CHECK_RANGE || opcode == OP::CHECK_RANGE_INVERTED) {
        length += Load4Bytes(instr + 1);
      }
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

  void Main() {
    // main pass
    Code::Data::const_pointer instr = first_instr_;
    const Code::Data::const_pointer last =
        code_.bytes().data() + code_.bytes().size();
    while (instr != last) {
      const uint8_t opcode = *instr;
      uint32_t length = kOPLength[opcode];
      if (opcode == OP::CHECK_RANGE || opcode == OP::CHECK_RANGE_INVERTED) {
        length += Load4Bytes(instr + 1);
      }

#define V(op, N)\
  case OP::op: {\
    const uint32_t offset = instr - first_instr_;\
    if (std::binary_search(targets_.begin(), targets_.end(), offset)) {\
      DefineLabel(offset);\
    }\
    const BackTrackMap::const_iterator it = backtracks_.find(offset);\
    if (it != backtracks_.end()) {\
      tracked_[it->second] = core::BitCast<uintptr_t>(getCurr());\
    }\
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

  void EmitPrologue() {
    // initialize capture buffer and put arguments registers
    push(rbp);
    mov(rbp, rsp);
    push(rdi);  // push vm to stack
    push(subject_);
    push(captures_);
    push(size_);
    push(cp_);
    push(sp_);

    // calling convension is
    // Execute(VM* vm, const char* subject,
    //         uint32_t size, int* captures, uint32_t cp)
    mov(subject_, rsi);
    mov(size_, rdx);
    mov(captures_, rcx);
    mov(cp_, r8);

    // initialize sp_ to 0
    mov(sp_, 0);
  }

  void EmitEpilogue() {
    // generate return path
    L(jit_detail::kReturnLabel);
    pop(sp_);
    pop(cp_);
    pop(size_);
    pop(captures_);
    pop(subject_);
    pop(r10);  // pop vm
    mov(rsp, rbp);
    pop(rbp);
    ret();
    align(16);

    // generate backtrack path
    inLocalLabel();
    L(jit_detail::kBackTrackLabel);
    test(sp_, sp_);
    jz(".ERROR");
    const int size = code_.captures() * 2 + code_.counters() + 1;
    sub(sp_, size);

    // copy to captures
    mov(r10, captures_);
    mov(r11, size);
    LoadStack(rax);
    add(rax, sp_);

    L(".LOOP_START");
    test(r11, r11);
    jz(".LOOP_END");
    dec(r11);
    mov(rdi, dword[rax]);
    mov(dword[r10], edi);
    add(r10, sizeof(int));  // NOLINT
    add(rax, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
    L(".LOOP_END");
    mov(cp_, dword[captures_ + sizeof(int)]);  // NOLINT
    mov(rdi, dword[captures_ + sizeof(int) * (size - 1)]);  // NOLINT
    mov(rax, core::BitCast<uintptr_t>(tracked_.data()));
    mov(rax, ptr[rax + rdi * sizeof(uintptr_t)]);
    jmp(rax);

    // generate error path
    L(".ERROR");
    Return(AERO_FAILURE);
    outLocalLabel();
  }

  void Return(int val) {
    mov(rax, val);
    jmp(jit_detail::kReturnLabel, T_NEAR);
  }

  void Return(const Xbyak::Reg64& reg) {
    mov(rax, reg);
    jmp(jit_detail::kReturnLabel, T_NEAR);
  }

  void EmitSizeGuard() {
    cmp(cp_, size_);
    jge(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitSTORE_SP(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(dword[captures_ + sizeof(int) * offset], spd_);  // NOLINT
  }

  void EmitSTORE_POSITION(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(dword[captures_ + sizeof(int) * offset], cpd_);  // NOLINT
  }

  void EmitPOSITION_TEST(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    cmp(cp_, dword[captures_ + sizeof(int) * offset]);  // NOLINT
    jne(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitASSERTION_SUCCESS(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(sp_, dword[captures_ + sizeof(int) * offset]);  // NOLINT
    LoadStack(r10);
    mov(cp_, dword[r10 + sizeof(int)]);  // NOLINT
    Jump(Load4Bytes(instr + 5));
  }

  void EmitASSERTION_FAILURE(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(sp_, dword[captures_ + sizeof(int) * offset]);  // NOLINT
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void InlineIsLineTerminator(const Xbyak::Reg64& reg, const char* ok) {
    cmp(reg, core::character::code::CR);
    je(ok);
    cmp(reg, core::character::code::LF);
    je(ok);
    cmp(reg, 0x2028);
    je(ok);
    cmp(reg, 0x2029);
    je(ok);
  }

  void EmitASSERTION_BOL(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    test(cp_, cp_);
    jz(".SUCCESS");
    mov(r10, character[subject_ + (cp_ * sizeof(CharT)) - (sizeof(CharT))]);
    InlineIsLineTerminator(r10, ".SUCCESS");
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
    mov(r10, character[subject_ + (cp_ * sizeof(CharT))]);
    InlineIsLineTerminator(r10, ".SUCCESS");
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    outLocalLabel();
  }

  void EmitASSERTION_EOB(const uint8_t* instr, uint32_t len) {
    cmp(cp_, size_);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void InlineIsWord(const Xbyak::Reg64& reg, const char* ok) {
    inLocalLabel();
    cmp(reg, '0');
    jl(".UNDERSCORE");
    cmp(reg, '9');
    jle(ok);
    L(".UNDERSCORE");
    cmp(reg, '_');
    je(ok);
    L(".ASCIIAlpha");
    or(reg, 0x20);
    cmp(reg, 'a');
    jl(".NG");
    cmp(reg, 'z');
    jle(ok);
    L(".NG");
    outLocalLabel();
  }

  void EmitASSERTION_WORD_BOUNDARY(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    test(cpd_, cpd_);
    jz(".FIRST_FALSE");
    cmp(cp_, size_);
    jg(".FIRST_FALSE");
    mov(r10, dword[subject_ + cp_ * sizeof(CharT) - sizeof(CharT)]);
    InlineIsWord(r10, ".FIRST_TRUE");
    jmp(".FIRST_FALSE");
    L(".FIRST_TRUE");
    mov(r10, 1);
    jmp(".SECOND");
    L(".FIRST_FALSE");
    mov(r10, 0);

    L(".SECOND");
    cmp(cp_, size_);
    jge(".SECOND_FALSE");
    mov(r11, dword[subject_ + cp_ * sizeof(CharT)]);
    InlineIsWord(r11, ".SECOND_TRUE");
    jmp(".SECOND_FALSE");
    L(".SECOND_TRUE");
    mov(r11, 1);
    jmp(".CHECK");
    L(".SECOND_FALSE");
    mov(r11, 0);

    L(".CHECK");
    cmp(r10, r11);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
    outLocalLabel();
  }

  void EmitASSERTION_WORD_BOUNDARY_INVERTED(const uint8_t* instr, uint32_t len) {  // NOLINT
    inLocalLabel();
    test(cpd_, cpd_);
    jz(".FIRST_FALSE");
    cmp(cp_, size_);
    jg(".FIRST_FALSE");
    mov(r10, dword[subject_ + cp_ * sizeof(CharT) - sizeof(CharT)]);
    InlineIsWord(r10, ".FIRST_TRUE");
    jmp(".FIRST_FALSE");
    L(".FIRST_TRUE");
    mov(r10, 1);
    jmp(".SECOND");
    L(".FIRST_FALSE");
    mov(r10, 0);

    L(".SECOND");
    cmp(cp_, size_);
    jge(".SECOND_FALSE");
    mov(r11, dword[subject_ + cp_ * sizeof(CharT)]);
    InlineIsWord(r11, ".SECOND_TRUE");
    jmp(".SECOND_FALSE");
    L(".SECOND_TRUE");
    mov(r11, 1);
    jmp(".CHECK");
    L(".SECOND_FALSE");
    mov(r11, 0);

    L(".CHECK");
    cmp(r10, r11);
    je(jit_detail::kBackTrackLabel, T_NEAR);
    outLocalLabel();
  }

  void EmitSTART_CAPTURE(const uint8_t* instr, uint32_t len) {
    const uint32_t target = Load4Bytes(instr + 1);
    mov(dword[captures_ + sizeof(int) * (target * 2)], cpd_);  // NOLINT
    mov(dword[captures_ + sizeof(int) * (target * 2 + 1)], kUndefined);  // NOLINT
  }

  void EmitEND_CAPTURE(const uint8_t* instr, uint32_t len) {
    const uint32_t target = Load4Bytes(instr + 1);
    mov(dword[captures_ + sizeof(int) * (target * 2 + 1)], cpd_);  // NOLINT
  }

  void EmitCLEAR_CAPTURES(const uint8_t* instr, uint32_t len) {
    const uint32_t from = Load4Bytes(instr + 1) * 2;
    const uint32_t to = Load4Bytes(instr + 5) * 2;
    if ((to - from) <= 10) {
      // loop unrolling
      if ((to - from) > 0) {
        mov(r10, captures_);
        add(r10, from);
        uint32_t i = from;
        while (true) {
          mov(dword[r10], -1);
          ++i;
          if (i == to) {
            break;
          }
          add(r10, sizeof(int));  // NOLINT
        }
      }
    } else {
      assert(to - from);  // not 0
      inLocalLabel();
      mov(r10, (to - from));
      mov(r11, captures_);
      L(".LOOP_START");
      mov(dword[r11], -1);
      add(r11, sizeof(int));  // NOLINT
      dec(r10);
      test(r10, r10);
      jnz(".LOOP_START");
      outLocalLabel();
    }
  }

  void EmitCOUNTER_ZERO(const uint8_t* instr, uint32_t len) {
    const uint32_t counter = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(dword[captures_ + sizeof(int) * counter], 0);  // NOLINT
  }

  void EmitCOUNTER_NEXT(const uint8_t* instr, uint32_t len) {
    const int max = static_cast<int>(Load4Bytes(instr + 5));
    const uint32_t counter = code_.captures() * 2 + Load4Bytes(instr + 1);
    mov(r10d, dword[captures_ + sizeof(int) * counter]);  // NOLINT
    inc(r10d);
    mov(dword[captures_ + sizeof(int) * counter], r10d);  // NOLINT
    cmp(r10, max);
    jl(MakeLabel(Load4Bytes(instr + 9)).c_str(), T_NEAR);
  }

  void EmitPUSH_BACKTRACK(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    const int size = code_.captures() * 2 + code_.counters() + 1;
    LoadVM(rdi);
    mov(rsi, sp_);
    mov(rdx, size);
    mov(rax, core::BitCast<uintptr_t>(&VM::NewStateForJIT));
    call(rax);
    test(rax, rax);
    jnz(".SUCCESS", T_NEAR);
    Return(AERO_ERROR);
    L(".SUCCESS");
    mov(sp_, rax);
    LoadStack(sp_);
    shr(sp_, sizeof(int) / 2);  // NOLINT

    // copy
    mov(r10, captures_);
    mov(r11, size - 1);
    mov(rsi, rax);

    L(".LOOP_START");
    test(r11, r11);
    jz(".LOOP_END");
    dec(r11);
    mov(rdi, dword[r10]);
    mov(dword[rax], edi);
    add(r10, sizeof(int));  // NOLINT
    add(rax, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
    L(".LOOP_END");
    const int val = static_cast<int>(Load4Bytes(instr + 1));
    const BackTrackMap::const_iterator it = backtracks_.find(val);
    assert(it != backtracks_.end());
    mov(dword[rax], it->second);
    mov(dword[rsi + sizeof(int)], cpd_);  // NOLINT
    outLocalLabel();
  }

  void EmitBACK_REFERENCE(const uint8_t* instr, uint32_t len) {
  }

  void EmitBACK_REFERENCE_IGNORE_CASE(const uint8_t* instr, uint32_t len) {
  }

  void EmitCHECK_1BYTE_CHAR(const uint8_t* instr, uint32_t len) {
    EmitSizeGuard();
    const CharT ch = Load1Bytes(instr + 1);
    cmp(character[subject_ + cp_ * sizeof(CharT)], ch);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
    inc(cp_);
  }

  void EmitCHECK_2BYTE_CHAR(const uint8_t* instr, uint32_t len) {
    if (sizeof(CharT) == 2) {
      EmitSizeGuard();
      const CharT ch = Load1Bytes(instr + 2);
      mov(r10, character[subject_ + cp_ * sizeof(CharT)]);
      cmp(r10, ch);
      jne(jit_detail::kBackTrackLabel, T_NEAR);
      inc(cp_);
    } else {
      jmp(jit_detail::kBackTrackLabel, T_NEAR);
    }
  }

  void EmitCHECK_2CHAR_OR(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    mov(r10, character[subject_ + cp_ * sizeof(CharT)]);
    cmp(r10, Load2Bytes(instr + 1));
    je(".SUCCESS");
    cmp(r10, Load2Bytes(instr + 3));
    je(".SUCCESS");
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitCHECK_3CHAR_OR(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    mov(r10, character[subject_ + cp_ * sizeof(CharT)]);
    cmp(r10, Load2Bytes(instr + 1));
    je(".SUCCESS");
    cmp(r10, Load2Bytes(instr + 3));
    je(".SUCCESS");
    cmp(r10, Load2Bytes(instr + 5));
    je(".SUCCESS");
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitCHECK_RANGE(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    mov(r10, character[subject_ + cp_ * sizeof(CharT)]);
    const uint32_t length = Load4Bytes(instr + 1);
    for (std::size_t i = 0; i < length; i += 4) {
      const uint16_t start = Load2Bytes(instr + 1 + 4 + i);
      cmp(r10, start);
      jl(jit_detail::kBackTrackLabel, T_NEAR);
      const uint16_t finish = Load2Bytes(instr + 1 + 4 + i + 2);
      cmp(r10, finish);
      jle(".SUCCESS");
    }
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitCHECK_RANGE_INVERTED(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    mov(r10, character[subject_ + cp_ * sizeof(CharT)]);
    const uint32_t length = Load4Bytes(instr + 1);
    for (std::size_t i = 0; i < length; i += 4) {
      const uint16_t start = Load2Bytes(instr + 1 + 4 + i);
      cmp(r10, start);
      jl(".SUCCESS");
      const uint16_t finish = Load2Bytes(instr + 1 + 4 + i + 2);
      cmp(r10, finish);
      jle(jit_detail::kBackTrackLabel, T_NEAR);
    }
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitJUMP(const uint8_t* instr, uint32_t len) {
    Jump(Load4Bytes(instr + 1));
  }

  void EmitFAILURE(const uint8_t* instr, uint32_t len) {
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitSUCCESS(const uint8_t* instr, uint32_t len) {
    mov(dword[captures_ + sizeof(int)], cpd_);  // NOLINT
    Return(AERO_SUCCESS);
  }

  std::string MakeLabel(uint32_t num) {
    std::string str("AERO_");
    core::UInt32ToString(num, std::back_inserter(str));
    return str;
  }

  void DefineLabel(uint32_t num) {
    L(MakeLabel(num).c_str());
  }

  void Jump(uint32_t num) {
    jmp(MakeLabel(num).c_str(), T_NEAR);
  }

  const Code& code_;
  const Code::Data::const_pointer first_instr_;
  core::SortedVector<uint32_t> targets_;
  BackTrackMap backtracks_;
  std::vector<uintptr_t> tracked_;

  const Xbyak::AddressFrame character;  // NOLINT
  const Xbyak::Reg64& subject_;
  const Xbyak::Reg64& size_;
  const Xbyak::Reg64& captures_;
  const Xbyak::Reg64& cp_;
  const Xbyak::Reg32& cpd_;
  const Xbyak::Reg64& sp_;
  const Xbyak::Reg32& spd_;
};

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_H_
