#include <iv/platform.h>
#if !defined(IV_ENABLE_JIT)
#include <iv/dummy_cc.h>
IV_DUMMY_CC()
#else

#include <iv/debug.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/instruction.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/helper.h>
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/type.h>
#include <iv/lv5/breaker/stub.h>
#include <iv/lv5/breaker/compiler.h>
namespace iv {
namespace lv5 {
namespace breaker {

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_LSHIFT(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Lshift(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type.IsNotInt32() || rhs_type.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_LSHIFT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->sal(eax, rhs_value & 0x1F);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rcx, ".GENERIC");
    asm_->sal(eax, cl);
  }
  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_LSHIFT);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_RSHIFT(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Rshift(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type.IsNotInt32() || rhs_type.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_RSHIFT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->sar(eax, rhs_value & 0x1F);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rcx, ".GENERIC");
    asm_->sar(eax, cl);
  }
  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_RSHIFT);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

void Compiler::EmitBINARY_RSHIFT_LOGICAL(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::RshiftLogical(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  // lhs or rhs are not int32_t
  if (lhs_type.IsNotInt32() || rhs_type.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->shr(eax, rhs_value & 0x1F);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rcx, ".GENERIC");
    asm_->shr(eax, cl);
  }
  // eax MSB is 1 => jump
  asm_->test(eax, eax);
  asm_->js(".DOUBLE");  // uint32_t

  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".DOUBLE");
  asm_->cvtsi2sd(xmm0, rax);
  asm_->movq(rax, xmm0);
  ConvertDoubleToJSVal(rax);
  asm_->jmp(".EXIT");

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_BIT_AND(const Instruction* instr, OP::Type fused) {
  const register_t lhs =
      Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
  const register_t rhs =
      Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
  const register_t dst = Reg(instr[1].i16[0]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::BitwiseAnd(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    if (fused != OP::NOP) {
      // fused jump opcode
      const Xbyak::Label& label = LookupLabel(instr);
      const bool result = dst_type.constant().ToBoolean();
      if ((fused == OP::IF_TRUE) == result) {
        asm_->jmp(label, Xbyak::CodeGenerator::T_NEAR);
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
    asm_->Call(&stub::BINARY_BIT_AND);
    if (fused != OP::NOP) {
      const Xbyak::Label& label = LookupLabel(instr);
      asm_->test(eax, eax);
      if (fused == OP::IF_TRUE) {
        asm_->jnz(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jz(label, Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, dst_type);
    }
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type.constant().int32();
    LoadVR(rax, rhs);
    Int32Guard(rhs, rax, ".GENERIC");
    asm_->and(eax, lhs_value);
  } else if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->and(eax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rdx, ".GENERIC");
    asm_->and(eax, edx);
  }

  if (fused != OP::NOP) {
    // fused jump opcode
    const Xbyak::Label& label = LookupLabel(instr);
    if (fused == OP::IF_TRUE) {
      asm_->jnz(label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jz(label, Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->jmp(".EXIT");

    kill_last_used();

    asm_->L(".GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_AND);

    asm_->test(eax, eax);
    if (fused == OP::IF_TRUE) {
      asm_->jnz(label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jz(label, Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->L(".EXIT");
  } else {
    // boxing
    asm_->or(rax, r15);
    asm_->jmp(".EXIT");

    kill_last_used();

    asm_->L(".GENERIC");
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_AND);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
  }
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_BIT_XOR(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::BitwiseXor(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  // lhs or rhs is not int32_t
  if (lhs_type.IsNotInt32() || rhs_type.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_XOR);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type.constant().int32();
    LoadVR(rax, rhs);
    Int32Guard(rhs, rax, ".GENERIC");
    asm_->xor(eax, lhs_value);
  } else if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->xor(eax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rdx, ".GENERIC");
    asm_->xor(eax, edx);
  }
  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_BIT_XOR);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_BIT_OR(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::BitwiseOr(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  // lhs or rhs is not int32_t
  if (lhs_type.IsNotInt32() || rhs_type.IsNotInt32()) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_BIT_OR);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type.constant().int32();
    LoadVR(rax, rhs);
    Int32Guard(rhs, rax, ".GENERIC");
    asm_->or(rax, lhs_value);
  } else if (rhs_type.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->or(rax, rhs_value);
  } else {
    LoadVRs(rax, lhs, rdx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rdx, ".GENERIC");
    asm_->or(rax, rdx);
  }
  // boxing, but because of 'or', we can remove boxing or phase.
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_BIT_OR);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

} } }  // namespace iv::lv5::breaker
#endif
