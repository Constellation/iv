#ifndef IV_LV5_BREAKER_COMPILER_COMPARISON_H_
#define IV_LV5_BREAKER_COMPILER_COMPARISON_H_
namespace iv {
namespace lv5 {
namespace breaker {

template<class Derived, Rep (*STUB)(Frame* stack, JSVal lhs, JSVal rhs)>
struct CompareTraits {
  typedef Rep (*Stub)(Frame* stack, JSVal lhs, JSVal rhs);
  static const Stub kStub;
};

template<class Derived, Rep (*STUB)(Frame* stack, JSVal lhs, JSVal rhs)>
const typename CompareTraits<Derived, STUB>::Stub
  CompareTraits<Derived, STUB>::kStub = STUB;

struct LTTraits : public CompareTraits<LTTraits, stub::BINARY_LT> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::LT(lhs, rhs);
  }

  static void JumpInt(Assembler* assembler, bool if_true, const char* label) {
    if (if_true) {
      assembler->jl(label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jge(label, Xbyak::CodeGenerator::T_NEAR);
    }
  }

  static void SetFlagInt(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setl(reg);
  }

  static void CompareDoubleAndOperation(
      Assembler* assembler,
      bool if_true,
      const Xbyak::Xmm& fp0,
      const Xbyak::Xmm& fp1,
      const Xbyak::Reg8& reg,
      const char* label = nullptr) {
    assembler->ucomisd(fp1, fp0);  // inverted
    if (label) {
      if (if_true) {
        assembler->ja(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jbe(label, Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      assert(if_true == true);
      assembler->seta(reg);
    }
  }
};

struct LTETraits : public CompareTraits<LTETraits, stub::BINARY_LTE> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::LTE(lhs, rhs);
  }

  static void JumpInt(Assembler* assembler, bool if_true, const char* label) {
    if (if_true) {
      assembler->jle(label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jg(label, Xbyak::CodeGenerator::T_NEAR);
    }
  }

  static void SetFlagInt(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setle(reg);
  }

  static void CompareDoubleAndOperation(
      Assembler* assembler,
      bool if_true,
      const Xbyak::Xmm& fp0,
      const Xbyak::Xmm& fp1,
      const Xbyak::Reg8& reg,
      const char* label = nullptr) {
    assembler->ucomisd(fp1, fp0);  // inverted
    if (label) {
      if (if_true) {
        assembler->jae(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jb(label, Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      assert(if_true == true);
      assembler->setae(reg);
    }
  }
};

struct GTTraits : public CompareTraits<GTTraits, stub::BINARY_GT> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::GT(lhs, rhs);
  }

  static void JumpInt(Assembler* assembler, bool if_true, const char* label) {
    if (if_true) {
      assembler->jg(label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jle(label, Xbyak::CodeGenerator::T_NEAR);
    }
  }

  static void SetFlagInt(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setg(reg);
  }

  static void CompareDoubleAndOperation(
      Assembler* assembler,
      bool if_true,
      const Xbyak::Xmm& fp0,
      const Xbyak::Xmm& fp1,
      const Xbyak::Reg8& reg,
      const char* label = nullptr) {
    assembler->ucomisd(fp0, fp1);
    if (label) {
      if (if_true) {
        assembler->ja(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jbe(label, Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      assert(if_true == true);
      assembler->seta(reg);
    }
  }
};

struct GTETraits : public CompareTraits<GTETraits, stub::BINARY_GTE> {
  static TypeEntry TypeAnalysis(const TypeEntry& lhs, const TypeEntry& rhs) {
    return TypeEntry::GTE(lhs, rhs);
  }

  static void JumpInt(Assembler* assembler, bool if_true, const char* label) {
    if (if_true) {
      assembler->jge(label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jl(label, Xbyak::CodeGenerator::T_NEAR);
    }
  }

  static void SetFlagInt(Assembler* assembler, const Xbyak::Reg8& reg) {
    assembler->setge(reg);
  }

  static void CompareDoubleAndOperation(
      Assembler* assembler,
      bool if_true,
      const Xbyak::Xmm& fp0,
      const Xbyak::Xmm& fp1,
      const Xbyak::Reg8& reg,
      const char* label = nullptr) {
    assembler->ucomisd(fp0, fp1);
    if (label) {
      if (if_true) {
        assembler->jae(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jb(label, Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      assert(if_true == true);
      assembler->setae(reg);
    }
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

  const std::string label = (fused != OP::NOP) ? MakeLabel(instr) : "";
  // (fused == OP::IF_TRUE || fused == OP::NOP)
  const bool jump_if_true = fused != OP::IF_FALSE;

  // dst is constant
  if (dst_type.IsConstant()) {
    if (fused != OP::NOP) {
      // fused jump opcode
      const bool result = dst_type.constant().ToBoolean();
      if (jump_if_true == result) {
        asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      EmitConstantDest(dst_type, dst);
      type_record_.Put(dst, dst_type);
    }
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  // lhs or rhs are not int32_t
  if (!(lhs_type.IsNotInt32() || rhs_type.IsNotInt32())) {
    if (rhs_type.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type.constant().int32();
      LoadVR(rax, lhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      asm_->cmp(eax, rhs_value);
    } else if (lhs_type.IsConstantInt32()) {
      const int32_t lhs_value = lhs_type.constant().int32();
      LoadVR(rax, rhs);
      Int32Guard(rhs, rax, ".DOUBLE");
      asm_->mov(edx, lhs_value);
      asm_->cmp(edx, eax);
    } else {
      LoadVRs(rax, lhs, rdx, rhs);
      Int32Guard(lhs, rax, ".DOUBLE");
      Int32Guard(rhs, rdx, ".DOUBLE");
      asm_->cmp(eax, edx);
    }

    // Compare int32s.
    if (fused != OP::NOP) {
      Traits::JumpInt(asm_, jump_if_true, label.c_str());
    } else {
      // set flag & boxing
      Traits::SetFlagInt(asm_, cl);
      ConvertBooleanToJSVal(cl, rax);
    }
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  kill_last_used();
  asm_->L(".DOUBLE"); {
    LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
    LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
    // Compare & perform operations on doubles.
    Traits::CompareDoubleAndOperation(
        asm_,
        jump_if_true,
        xmm0,
        xmm1,
        cl,
        fused != OP::NOP ? label.c_str() : nullptr
        );
    if (fused != OP::NOP) {
      // Do nothing. Already jumped.
    } else {
      // boxing
      ConvertBooleanToJSVal(cl, rax);
    }
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".GENERIC"); {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);
    if (fused != OP::NOP) {
      asm_->cmp(rax, Extract(JSTrue));
      if (jump_if_true) {
        asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      // Do nothing.
    }
    // Through to EXIT.
  }

  asm_->L(".EXIT"); {
    if (fused != OP::NOP) {
      // Do nothing.
    } else {
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, dst_type);
    }
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
