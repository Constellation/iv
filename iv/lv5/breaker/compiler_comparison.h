#ifndef IV_LV5_BREAKER_COMPILER_COMPARISON_H_
#define IV_LV5_BREAKER_COMPILER_COMPARISON_H_
namespace iv {
namespace lv5 {
namespace breaker {

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

  const Xbyak::Label* label =
      (fused != OP::NOP) ? &LookupLabel(instr) : nullptr;
  // (fused == OP::IF_TRUE || fused == OP::NOP)
  const bool jump_if_true = fused != OP::IF_FALSE;

  // dst is constant
  if (dst_type.IsConstant()) {
    if (fused != OP::NOP) {
      // fused jump opcode
      const bool result = dst_type.constant().ToBoolean();
      if (jump_if_true == result) {
        asm_->jmp(*label, Xbyak::CodeGenerator::T_NEAR);
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
      Traits::JumpInt(asm_, jump_if_true, label);
    } else {
      // set flag & boxing
      Traits::SetFlagInt(asm_, cl);
      ConvertBooleanToJSVal(cl, rax);
    }
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  kill_last_used();

  Xbyak::Label generic;
  asm_->L(".DOUBLE"); {
    LoadDouble(lhs, xmm0, rsi, &generic);
    LoadDouble(rhs, xmm1, rsi, &generic);
    // Compare & perform operations on doubles.
    Traits::CompareDoubleAndOperation(
        asm_,
        jump_if_true,
        xmm0,
        xmm1,
        cl,
        label
        );
    if (fused != OP::NOP) {
      // Do nothing. Already jumped.
    } else {
      // boxing
      ConvertBooleanToJSVal(cl, rax);
    }
    asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(generic); {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(Traits::kStub);
    if (fused != OP::NOP) {
      asm_->cmp(rax, Extract(JSTrue));
      if (jump_if_true) {
        asm_->je(*label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(*label, Xbyak::CodeGenerator::T_NEAR);
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

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_COMPARISON_H_
