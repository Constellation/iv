#ifndef IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
#define IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
namespace iv {
namespace lv5 {
namespace breaker {

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_MULTIPLY(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Multiply(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (!(lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32())) {
    if (lhs_type_entry.IsConstantInt32()) {
      const int32_t lhs_value = lhs_type_entry.constant().int32();
      LoadVR(rax, rhs);
      Int32Guard(rhs, rax, ".DOUBLE");
      asm_->imul(eax, eax, lhs_value);
      asm_->jo(".OVERFLOW");
    } else if (rhs_type_entry.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type_entry.constant().int32();
      LoadVR(rax, lhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      asm_->imul(eax, eax, rhs_value);
      asm_->jo(".OVERFLOW");
    } else {
      LoadVRs(rax, lhs, rdx, rhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      Int32Guard(rhs, rdx, ".DOUBLE");
      asm_->imul(eax, edx);
      asm_->jo(".OVERFLOW");
    }
    // boxing
    asm_->or(rax, r15);
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);

    kill_last_used();

    // lhs and rhs are always int32 (but overflow)
    asm_->L(".OVERFLOW");
    LoadVRs(rax, lhs, rdx, rhs);
    asm_->cvtsi2sd(xmm0, eax);
    asm_->cvtsi2sd(xmm1, edx);
    asm_->mulsd(xmm0, xmm1);
    asm_->movq(rax, xmm0);
    ConvertDoubleToJSVal(rax);
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".DOUBLE");
  LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
  LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);

  asm_->mulsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_MULTIPLY);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_DIVIDE(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Divide(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  LoadDouble(lhs, xmm0, rsi, ".GENERIC");
  LoadDouble(rhs, xmm1, rsi, ".GENERIC");

  asm_->divsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_DIVIDE);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_ADD(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Add(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (!(lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32())) {
    if (lhs_type_entry.IsConstantInt32()) {
      const int32_t lhs_value = lhs_type_entry.constant().int32();
      LoadVR(rax, rhs);
      Int32Guard(rhs, rax, ".DOUBLE");
      asm_->add(eax, lhs_value);
      asm_->jo(".OVERFLOW");
    } else if (rhs_type_entry.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type_entry.constant().int32();
      LoadVR(rax, lhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      asm_->add(eax, rhs_value);
      asm_->jo(".OVERFLOW");
    } else {
      LoadVRs(rax, lhs, rdx, rhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      Int32Guard(rhs, rdx, ".DOUBLE");
      asm_->add(eax, edx);
      asm_->jo(".OVERFLOW");
    }
    // boxing
    asm_->or(rax, r15);
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);

    kill_last_used();

    // lhs and rhs are always int32 (but overflow)
    asm_->L(".OVERFLOW");
    LoadVRs(rax, lhs, rdx, rhs);
    asm_->movsxd(rax, eax);
    asm_->movsxd(rdx, edx);
    asm_->add(rax, rdx);
    asm_->cvtsi2sd(xmm0, rax);
    asm_->movq(rax, xmm0);
    ConvertDoubleToJSVal(rax);
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".DOUBLE");
  LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
  LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);

  asm_->addsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_ADD);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_MODULO(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Modulo(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  // lhs or rhs are not int32_t
  //
  // OR
  //
  // check rhs is more than 0 (n % 0 == NaN)
  // lhs is >= 0 and rhs is > 0 because example like
  //   -1 % -1
  // should return -0.0, so this value is double
  if (lhs_type_entry.IsNotInt32() ||
      rhs_type_entry.IsNotInt32() ||
      (rhs_type_entry.IsConstantInt32() &&
       rhs_type_entry.constant().int32() <= 0) ||
      (lhs_type_entry.IsConstantInt32() &&
       lhs_type_entry.constant().int32() < 0)) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_MODULO);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  // Because LHS and RHS are not signed,
  // we should store 0 to edx
  if (lhs_type_entry.IsConstantInt32()) {
    // RHS > 0
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(rcx, rhs);
    Int32Guard(rhs, rcx, ".ARITHMETIC_GENERIC");
    asm_->test(ecx, ecx);
    asm_->jle(".ARITHMETIC_GENERIC");
    asm_->xor(edx, edx);
    asm_->mov(eax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    // LHS >= 0
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->test(eax, eax);
    asm_->js(".ARITHMETIC_GENERIC");
    asm_->xor(edx, edx);
    asm_->mov(ecx, rhs_value);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rcx, ".ARITHMETIC_GENERIC");
    asm_->test(eax, eax);
    asm_->js(".ARITHMETIC_GENERIC");
    asm_->test(ecx, ecx);
    asm_->jle(".ARITHMETIC_GENERIC");
    asm_->xor(edx, edx);
  }
  asm_->idiv(ecx);
  asm_->mov(eax, edx);

  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_MODULO);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_LSHIFT(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Lshift(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_LSHIFT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->sal(eax, rhs_value & 0x1F);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rcx, ".ARITHMETIC_GENERIC");
    asm_->sal(eax, cl);
  }
  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_LSHIFT);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_RSHIFT(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Rshift(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_RSHIFT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->sar(eax, rhs_value & 0x1F);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rcx, ".ARITHMETIC_GENERIC");
    asm_->sar(eax, cl);
  }
  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_RSHIFT);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

inline void Compiler::EmitBINARY_RSHIFT_LOGICAL(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::RshiftLogical(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->shr(eax, rhs_value & 0x1F);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rcx, ".ARITHMETIC_GENERIC");
    asm_->shr(eax, cl);
  }
  // eax MSB is 1 => jump
  asm_->test(eax, eax);
  asm_->js(".ARITHMETIC_DOUBLE");  // uint32_t

  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_DOUBLE");
  asm_->cvtsi2sd(xmm0, rax);
  asm_->movq(rax, xmm0);
  ConvertDoubleToJSVal(rax);
  asm_->jmp(".ARITHMETIC_EXIT");

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

template<Rep (*STUB)(Frame* stack, JSVal lhs, JSVal rhs)>
struct CompareTraits {
  typedef Rep (*Stub)(Frame* stack, JSVal lhs, JSVal rhs);
  static const Stub kStub;
};

template<Rep (*STUB)(Frame* stack, JSVal lhs, JSVal rhs)>
const typename CompareTraits<STUB>::Stub CompareTraits<STUB>::kStub = STUB;

struct LTTraits : public CompareTraits<stub::BINARY_LT> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::LT(lhs, rhs);
  }
  static void JumpIfTrue(Assembler* assembler, const char* label) {
    assembler->jl(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void JumpIfFalse(Assembler* assembler, const char* label) {
    assembler->jge(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void SetFlag(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setl(reg);
  }
};

struct LTETraits : public CompareTraits<stub::BINARY_LTE> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::LTE(lhs, rhs);
  }
  static void JumpIfTrue(Assembler* assembler, const char* label) {
    assembler->jle(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void JumpIfFalse(Assembler* assembler, const char* label) {
    assembler->jg(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void SetFlag(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setle(reg);
  }
};

struct GTTraits : public CompareTraits<stub::BINARY_GT> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::GT(lhs, rhs);
  }
  static void JumpIfTrue(Assembler* assembler, const char* label) {
    assembler->jg(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void JumpIfFalse(Assembler* assembler, const char* label) {
    assembler->jle(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void SetFlag(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setg(reg);
  }
};

struct GTETraits : public CompareTraits<stub::BINARY_GTE> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::GTE(lhs, rhs);
  }
  static void JumpIfTrue(Assembler* assembler, const char* label) {
    assembler->jge(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void JumpIfFalse(Assembler* assembler, const char* label) {
    assembler->jl(label, Xbyak::CodeGenerator::T_NEAR);
  }
  static void SetFlag(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setge(reg);
  }
};

template<typename Traits>
inline void Compiler::EmitCompare(const Instruction* instr, OP::Type fused) {
  const register_t lhs =
      Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
  const register_t rhs =
      Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
  const register_t dst = Reg(instr[1].i16[0]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      Traits::TypeAnalysis(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      const bool result = dst_type_entry.constant().ToBoolean();
      if ((fused == OP::IF_TRUE) == result) {
        asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      EmitConstantDest(dst_type_entry, dst);
      type_record_.Put(dst, dst_type_entry);
    }
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);
    if (fused != OP::NOP) {
      const std::string label = MakeLabel(instr);
      asm_->cmp(rax, Extract(JSTrue));
      if (fused == OP::IF_TRUE) {
        asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, dst_type_entry);
    }
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->cmp(eax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rdx, ".ARITHMETIC_GENERIC");
    asm_->cmp(eax, edx);
  }

  if (fused != OP::NOP) {
    // fused jump opcode
    const std::string label = MakeLabel(instr);
    if (fused == OP::IF_TRUE) {
      Traits::JumpIfTrue(asm_, label.c_str());
    } else {
      Traits::JumpIfFalse(asm_, label.c_str());
    }
    asm_->jmp(".ARITHMETIC_EXIT");

    kill_last_used();

    asm_->L(".ARITHMETIC_GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);

    asm_->cmp(rax, Extract(JSTrue));
    if (fused == OP::IF_TRUE) {
      asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->L(".ARITHMETIC_EXIT");
  } else {
    // boxing
    Traits::SetFlag(asm_, cl);
    ConvertBooleanToJSVal(cl, rax);
    asm_->jmp(".ARITHMETIC_EXIT");

    kill_last_used();

    asm_->L(".ARITHMETIC_GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);

    asm_->L(".ARITHMETIC_EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
  }
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_LT(const Instruction* instr, OP::Type fused) {
  EmitCompare<LTTraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_LTE(const Instruction* instr, OP::Type fused) {
  EmitCompare<LTETraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_GT(const Instruction* instr, OP::Type fused) {
  EmitCompare<GTTraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_GTE(const Instruction* instr, OP::Type fused) {
  EmitCompare<GTETraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_SUBTRACT(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Subtract(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (!(lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32())) {
    if (rhs_type_entry.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type_entry.constant().int32();
      LoadVR(rax, lhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      asm_->sub(eax, rhs_value);
      asm_->jo(".OVERFLOW");
    } else {
      LoadVRs(rax, lhs, rdx, rhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      Int32Guard(rhs, rdx, ".DOUBLE");
      asm_->sub(eax, edx);
      asm_->jo(".OVERFLOW");
    }
    // boxing
    asm_->or(rax, r15);
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);

    kill_last_used();

    // lhs and rhs are always int32 (but overflow)
    // So we just sub as int64_t and convert to double,
    // because INT32_MIN - INT32_MIN is in int64_t range, and convert to
    // double makes no error.
    asm_->L(".OVERFLOW");
    LoadVRs(rax, lhs, rdx, rhs);
    asm_->movsxd(rax, eax);
    asm_->movsxd(rdx, edx);
    asm_->sub(rax, rdx);
    asm_->cvtsi2sd(xmm0, rax);
    asm_->movq(rax, xmm0);
    ConvertDoubleToJSVal(rax);
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".DOUBLE");
  LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
  LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);

  asm_->subsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_SUBTRACT);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_BIT_AND(const Instruction* instr,
                                          OP::Type fused) {
  const register_t lhs =
      Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
  const register_t rhs =
      Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
  const register_t dst = Reg(instr[1].i16[0]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::BitwiseAnd(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      const bool result = dst_type_entry.constant().ToBoolean();
      if ((fused == OP::IF_TRUE) == result) {
        asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      EmitConstantDest(dst_type_entry, dst);
      type_record_.Put(dst, dst_type_entry);
    }
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_AND);
    if (fused != OP::NOP) {
      const std::string label = MakeLabel(instr);
      asm_->test(eax, eax);
      if (fused == OP::IF_TRUE) {
        asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, dst_type_entry);
    }
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(rax, rhs);
    Int32Guard(rhs, rax, ".ARITHMETIC_GENERIC");
    asm_->and(eax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->and(eax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rdx, ".ARITHMETIC_GENERIC");
    asm_->and(eax, edx);
  }

  if (fused != OP::NOP) {
    // fused jump opcode
    const std::string label = MakeLabel(instr);
    if (fused == OP::IF_TRUE) {
      asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->jmp(".ARITHMETIC_EXIT");

    kill_last_used();

    asm_->L(".ARITHMETIC_GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_AND);

    asm_->test(eax, eax);
    if (fused == OP::IF_TRUE) {
      asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->L(".ARITHMETIC_EXIT");
  } else {
    // boxing
    asm_->or(rax, r15);
    asm_->jmp(".ARITHMETIC_EXIT");

    kill_last_used();

    asm_->L(".ARITHMETIC_GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_AND);

    asm_->L(".ARITHMETIC_EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
  }
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_BIT_XOR(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::BitwiseXor(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  // lhs or rhs is not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_XOR);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(rax, rhs);
    Int32Guard(rhs, rax, ".ARITHMETIC_GENERIC");
    asm_->xor(eax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->xor(eax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rdx, ".ARITHMETIC_GENERIC");
    asm_->xor(eax, edx);
  }
  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_BIT_XOR);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_BIT_OR(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::BitwiseOr(lhs_type_entry, rhs_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  // lhs or rhs is not int32_t
  if (lhs_type_entry.IsNotInt32() || rhs_type_entry.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_OR);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(rax, rhs);
    Int32Guard(rhs, rax, ".ARITHMETIC_GENERIC");
    asm_->or(rax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    asm_->or(rax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, rdx, ".ARITHMETIC_GENERIC");
    asm_->or(rax, rdx);
  }
  // boxing, but because of 'or', we can remove boxing or phase.
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_BIT_OR);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
