#ifndef IV_LV5_BREAKER_COMPILER_COMPARISON_H_
#define IV_LV5_BREAKER_COMPILER_COMPARISON_H_
namespace iv {
namespace lv5 {
namespace breaker {

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

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = Traits::TypeAnalysis(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      const bool result = dst_type.constant().ToBoolean();
      if ((fused == OP::IF_TRUE) == result) {
        asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      EmitConstantDest(dst_type, dst);
      type_record_.Put(dst, dst_type);
    }
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type.IsNotInt32() || rhs_type.IsNotInt32()) {
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
      type_record_.Put(dst, dst_type);
    }
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->cmp(eax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rdx, ".GENERIC");
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
    asm_->jmp(".EXIT");

    kill_last_used();

    asm_->L(".GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);

    asm_->cmp(rax, Extract(JSTrue));
    if (fused == OP::IF_TRUE) {
      asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->L(".EXIT");
  } else {
    // boxing
    Traits::SetFlag(asm_, cl);
    ConvertBooleanToJSVal(cl, rax);
    asm_->jmp(".EXIT");

    kill_last_used();

    asm_->L(".GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
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

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_COMPARISON_H_
