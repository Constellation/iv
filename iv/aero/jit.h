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
#include <iv/aero/jit_fwd.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/xbyak/xbyak.h>

namespace iv {
namespace aero {
namespace jit_detail {

static const char* kBackTrackLabel = "BACKTRACK";
static const char* kReturnLabel = "RETURN";

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
struct Reg<uint16_t> {
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

  static const int kVM = 0;

  explicit JIT(const Code& code)
    : Xbyak::CodeGenerator(8192 * 16),
      code_(code),
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
      spd_(ebx),
      ch10_(jit_detail::Reg<CharT>::GetR10(this)),
      ch11_(jit_detail::Reg<CharT>::GetR11(this)) { }

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

  void LoadResult(const Xbyak::Reg64& dst) {
    mov(dst, ptr[rbp - 16]);
  }

  void LoadStack(const Xbyak::Reg64& dst) {
    LoadVM(dst);
    mov(dst, ptr[dst]);
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

#define INTERCEPT()\
  do {\
    mov(r10, offset);\
    Put("OPCODE", r10);\
  } while (0)

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
#undef INTERCEPT
      std::advance(instr, length);
    }
  }

  void EmitPrologue() {
    // initialize capture buffer and put arguments registers
    push(rbp);
    mov(rbp, rsp);
    push(rdi);  // push vm to stack
    push(rcx);  // push captures to stack
    push(rcx);  // push captures to stack
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
    LoadVM(rcx);
    mov(captures_, ptr[rcx + sizeof(int*)]);  // NOLINT
    mov(cp_, r8);

    // initialize sp_ to 0
    mov(sp_, 0);

    // initialize capture position
    mov(dword[captures_], cpd_);
  }

  void EmitEpilogue() {
    // generate return path
    L(jit_detail::kReturnLabel);
    pop(sp_);
    pop(cp_);
    pop(size_);
    pop(captures_);
    pop(subject_);
    pop(r10);  // pop captures
    pop(r10);  // pop captures
    pop(r10);  // pop vm
    mov(rsp, rbp);
    pop(rbp);
    ret();
    align(16);

    // generate backtrack path
    inLocalLabel();
    L(jit_detail::kBackTrackLabel);
    test(sp_, sp_);
    jz(".ERROR", T_NEAR);
    const int size = code_.captures() * 2 + code_.counters() + 1;
    sub(sp_, size);

    // copy to captures
    mov(r10, captures_);
    mov(r11, size);
    LoadStack(rax);
    lea(rax, ptr[rax + sp_ * sizeof(int)]);  // NOLINT

    L(".LOOP_START");
    test(r11, r11);
    jz(".LOOP_END");
    dec(r11);
    mov(ecx, dword[rax]);
    mov(dword[r10], ecx);
    add(r10, sizeof(int));  // NOLINT
    add(rax, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
    L(".LOOP_END");
    movsxd(cp_, dword[captures_ + sizeof(int)]);  // NOLINT
    movsxd(rcx, dword[captures_ + sizeof(int) * (size - 1)]);  // NOLINT
    mov(rax, core::BitCast<uintptr_t>(tracked_.data()));
    jmp(ptr[rax + rcx * sizeof(uintptr_t)]);

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
    cmp(cpd_, dword[captures_ + sizeof(int) * offset]);  // NOLINT
    jne(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void EmitASSERTION_SUCCESS(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    movsxd(sp_, dword[captures_ + sizeof(int) * offset]);  // NOLINT
    LoadStack(r10);
    movsxd(cp_, dword[r10 + sizeof(int)]);  // NOLINT
    Jump(Load4Bytes(instr + 5));
  }

  void EmitASSERTION_FAILURE(const uint8_t* instr, uint32_t len) {
    const uint32_t offset = code_.captures() * 2 + Load4Bytes(instr + 1);
    movsxd(sp_, dword[captures_ + sizeof(int) * offset]);  // NOLINT
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void InlineIsLineTerminator(const RegC& reg, const char* ok) {
    cmp(reg, core::character::code::CR);
    je(ok);
    cmp(reg, core::character::code::LF);
    je(ok);
    if (sizeof(CharT) == 2) {
      cmp(reg, 0x2028);
      je(ok);
      cmp(reg, 0x2029);
      je(ok);
    }
  }

  void EmitASSERTION_BOL(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    test(cp_, cp_);
    jz(".SUCCESS");
    mov(ch10_, character[subject_ + (cp_ * sizeof(CharT)) - (sizeof(CharT))]);
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
    mov(ch10_, character[subject_ + (cp_ * sizeof(CharT))]);
    InlineIsLineTerminator(ch10_, ".SUCCESS");
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    outLocalLabel();
  }

  void EmitASSERTION_EOB(const uint8_t* instr, uint32_t len) {
    cmp(cp_, size_);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
  }

  void InlineIsWord(const RegC& reg, uint32_t n, const char* ok) {
    if (sizeof(CharT) == 2) {
      // insert ASCII check
      test(reg, static_cast<uint16_t>(65408));  // (1111111110000000)2
      jnz(MakeLabel(n, ".NG").c_str());
    }
    cmp(reg, '0');
    jl(MakeLabel(n, ".UNDERSCORE").c_str());
    cmp(reg, '9');
    jle(ok);
    L(MakeLabel(n, ".UNDERSCORE").c_str());
    cmp(reg, '_');
    je(ok);
    L(MakeLabel(n, ".ASCIIAlpha").c_str());
    or(reg, 0x20);
    cmp(reg, 'a');
    jl(MakeLabel(n, ".NG").c_str());
    cmp(reg, 'z');
    jle(ok);
    L(MakeLabel(n, ".NG").c_str());
  }

  void EmitASSERTION_WORD_BOUNDARY(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    test(cpd_, cpd_);
    jz(".FIRST_FALSE");
    cmp(cp_, size_);
    jg(".FIRST_FALSE");
    mov(ch10_, dword[subject_ + cp_ * sizeof(CharT) - sizeof(CharT)]);
    InlineIsWord(ch10_, 1, ".FIRST_TRUE");
    jmp(".FIRST_FALSE");
    L(".FIRST_TRUE");
    mov(r10, 1);
    jmp(".SECOND");
    L(".FIRST_FALSE");
    mov(r10, 0);

    L(".SECOND");
    cmp(cp_, size_);
    jge(".SECOND_FALSE");
    mov(ch11_, dword[subject_ + cp_ * sizeof(CharT)]);
    InlineIsWord(ch11_, 2, ".SECOND_TRUE");
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
    mov(ch10_, dword[subject_ + cp_ * sizeof(CharT) - sizeof(CharT)]);
    InlineIsWord(ch10_, 1, ".FIRST_TRUE");
    jmp(".FIRST_FALSE");
    L(".FIRST_TRUE");
    mov(r10, 1);
    jmp(".SECOND");
    L(".FIRST_FALSE");
    mov(r10, 0);

    L(".SECOND");
    cmp(cp_, size_);
    jge(".SECOND_FALSE");
    mov(ch11_, dword[subject_ + cp_ * sizeof(CharT)]);
    InlineIsWord(ch11_, 2, ".SECOND_TRUE");
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
    cmp(r10d, max);
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

    L(".SUCCESS");
    add(sp_, size);
    sub(rax, sizeof(int) * (size));  // NOLINT

    // copy
    mov(r10, captures_);
    mov(r11, size - 1);
    mov(rsi, rax);

    L(".LOOP_START");
    test(r11, r11);
    jz(".LOOP_END");
    dec(r11);
    mov(ecx, dword[r10]);
    mov(dword[rax], ecx);
    add(r10, sizeof(int));  // NOLINT
    add(rax, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
    L(".LOOP_END");
    const int val = static_cast<int>(Load4Bytes(instr + 1));
    const BackTrackMap::const_iterator it = backtracks_.find(val);
    assert(it != backtracks_.end());
    mov(dword[rsi + sizeof(int)], cpd_);  // NOLINT
    mov(dword[rsi + sizeof(int) * (size - 1)], static_cast<uint32_t>(it->second));  // NOLINT
    outLocalLabel();
  }

  void EmitBACK_REFERENCE(const uint8_t* instr, uint32_t len) {
    const uint16_t ref = Load2Bytes(instr + 1);
    assert(ref != 0);  // limited by parser
    if (ref >= code_.captures()) {
      return;
    }

    inLocalLabel();
    movsxd(rax, dword[captures_ + sizeof(int) * (ref * 2 + 1)]);  // NOLINT
    cmp(rax, -1);
    je(".SUCCESS", T_NEAR);
    movsxd(r10, dword[captures_ + sizeof(int) * (ref * 2)]);  // NOLINT

    mov(rcx, rax);
    add(rcx, cp_);
    cmp(rcx, size_);
    jg(jit_detail::kBackTrackLabel, T_NEAR);

    // back reference check
    lea(rdx, ptr[subject_ + r10 * sizeof(CharT)]);
    lea(rcx, ptr[subject_ + cp_ * sizeof(CharT)]);
    add(rcx, rax);
    mov(r8, rax);

    L(".LOOP_START");
    test(r8, r8);
    jz(".LOOP_END");
    dec(r8);
    mov(ch10_, character[rdx]);
    mov(ch11_, character[rcx]);
    cmp(ch10_, ch11_);
    jne(jit_detail::kBackTrackLabel, T_NEAR);
    add(rdx, sizeof(int));  // NOLINT
    add(rcx, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
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
    movsxd(rax, dword[captures_ + sizeof(int) * (ref * 2 + 1)]);  // NOLINT
    cmp(rax, -1);
    je(".SUCCESS", T_NEAR);
    movsxd(r10, dword[captures_ + sizeof(int) * (ref * 2)]);  // NOLINT

    mov(rcx, rax);
    add(rcx, cp_);
    cmp(rcx, size_);
    jg(jit_detail::kBackTrackLabel, T_NEAR);

    // back reference check
    lea(rdx, ptr[subject_ + r10 * sizeof(CharT)]);
    lea(rcx, ptr[subject_ + cp_ * sizeof(CharT)]);
    add(rcx, rax);
    mov(r8, rax);

    L(".LOOP_START");
    test(r8, r8);
    jz(".LOOP_END");
    dec(r8);
    mov(ch10_, character[rdx]);
    mov(ch11_, character[rcx]);
    cmp(ch10_, ch11_);
    je(".COND_OK", T_NEAR);

    // used callar-save registers
    // r8 r10 r11 rdx rcx
    push(r8);
    push(r8);

    push(rdx);
    push(rcx);
    movsxd(rdi, ch10_);
    mov(r10, core::BitCast<uintptr_t>(&core::character::ToUpperCase));
    call(r10);
    pop(rcx);
    cmp(eax, character[rcx]);
    je(".CALL_COND_OK");

    pop(rdx);
    movsxd(rdi, character[rdx]);
    push(rdx);
    push(rcx);
    movsxd(rdi, ch10_);
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
    add(rdx, sizeof(int));  // NOLINT
    add(rcx, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
    L(".LOOP_END");
    add(cp_, rax);

    L(".SUCCESS");
    outLocalLabel();
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
      const uint16_t ch = Load1Bytes(instr + 2);
      mov(ch10_, character[subject_ + cp_ * sizeof(CharT)]);
      cmp(ch10_, ch);
      jne(jit_detail::kBackTrackLabel, T_NEAR);
      inc(cp_);
    } else {
      jmp(jit_detail::kBackTrackLabel, T_NEAR);
    }
  }

  void EmitCHECK_2CHAR_OR(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    mov(ch10_, character[subject_ + cp_ * sizeof(CharT)]);
    const uint16_t first = Load2Bytes(instr + 1);
    if (!(sizeof(CharT) == 1 && !core::character::IsASCII(first))) {
      cmp(ch10_, first);
      je(".SUCCESS");
    }
    const uint16_t second = Load2Bytes(instr + 3);
    if (!(sizeof(CharT) == 1 && !core::character::IsASCII(second))) {
      cmp(ch10_, second);
      je(".SUCCESS");
    }
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitCHECK_3CHAR_OR(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    mov(ch10_, character[subject_ + cp_ * sizeof(CharT)]);
    const uint16_t first = Load2Bytes(instr + 1);
    if (!(sizeof(CharT) == 1 && !core::character::IsASCII(first))) {
      cmp(ch10_, first);
      je(".SUCCESS");
    }
    const uint16_t second = Load2Bytes(instr + 3);
    if (!(sizeof(CharT) == 1 && !core::character::IsASCII(second))) {
      cmp(ch10_, second);
      je(".SUCCESS");
    }
    const uint16_t third = Load2Bytes(instr + 5);
    if (!(sizeof(CharT) == 1 && !core::character::IsASCII(third))) {
      cmp(ch10_, third);
      je(".SUCCESS");
    }
    jmp(jit_detail::kBackTrackLabel, T_NEAR);
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitCHECK_RANGE(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    xor(r10, r10);
    mov(ch10_, character[subject_ + cp_ * sizeof(CharT)]);
    const uint32_t length = Load4Bytes(instr + 1);
    for (std::size_t i = 0; i < length; i += 4) {
      const uint16_t start = Load2Bytes(instr + 1 + 4 + i);
      const uint16_t finish = Load2Bytes(instr + 1 + 4 + i + 2);
      if (sizeof(CharT) == 1 && (!core::character::IsASCII(start))) {
        jmp(jit_detail::kBackTrackLabel, T_NEAR);
        break;
      }
      cmp(r10, start);
      jl(jit_detail::kBackTrackLabel, T_NEAR);

      if (sizeof(CharT) == 1 && (!core::character::IsASCII(finish))) {
        jmp(".SUCCESS", T_NEAR);
        break;
      }
      cmp(r10, finish);
      jle(".SUCCESS", T_NEAR);
    }
    L(".SUCCESS");
    inc(cp_);
    outLocalLabel();
  }

  void EmitCHECK_RANGE_INVERTED(const uint8_t* instr, uint32_t len) {
    inLocalLabel();
    EmitSizeGuard();
    xor(r10, r10);
    mov(ch10_, character[subject_ + cp_ * sizeof(CharT)]);
    const uint32_t length = Load4Bytes(instr + 1);
    for (std::size_t i = 0; i < length; i += 4) {
      const uint16_t start = Load2Bytes(instr + 1 + 4 + i);
      const uint16_t finish = Load2Bytes(instr + 1 + 4 + i + 2);
      if (sizeof(CharT) == 1 && (!core::character::IsASCII(start))) {
        jmp(".SUCCESS", T_NEAR);
        break;
      }
      cmp(r10, start);
      jl(".SUCCESS", T_NEAR);

      if (sizeof(CharT) == 1 && (!core::character::IsASCII(finish))) {
        jmp(jit_detail::kBackTrackLabel, T_NEAR);
        break;
      }
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


    // copy to result
    LoadResult(r10);
    mov(r11, code_.captures() * 2);
    mov(rax, captures_);

    L(".LOOP_START");
    test(r11, r11);
    jz(".LOOP_END");
    dec(r11);
    mov(ecx, dword[rax]);
    mov(dword[r10], ecx);
    add(r10, sizeof(int));  // NOLINT
    add(rax, sizeof(int));  // NOLINT
    jmp(".LOOP_START");
    L(".LOOP_END");

    Return(AERO_SUCCESS);
  }

  std::string MakeLabel(uint32_t num, const std::string& prefix = "AERO_") {
    std::string str(prefix);
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
  const RegC& ch10_;
  const RegC& ch11_;
};

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_H_
