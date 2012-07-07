#ifndef IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
#define IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
namespace iv {
namespace lv5 {
namespace breaker {

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_MULTIPLY(const Instruction* instr) {
  const int16_t dst = Reg(instr[1].i16[0]);
  const int16_t lhs = Reg(instr[1].i16[1]);
  const int16_t rhs = Reg(instr[1].i16[2]);

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

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() || rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_MULTIPLY);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(asm_->rax, rhs);
    Int32Guard(rhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->imul(asm_->eax, asm_->eax, lhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->imul(asm_->eax, asm_->eax, rhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->imul(asm_->eax, asm_->edx);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  // lhs and rhs is always int32 (but overflow)
  asm_->L(".ARITHMETIC_OVERFLOW");
  LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
  asm_->cvtsi2sd(asm_->xmm0, asm_->eax);
  asm_->cvtsi2sd(asm_->xmm1, asm_->edx);
  asm_->mulsd(asm_->xmm0, asm_->xmm1);
  asm_->movq(asm_->rax, asm_->xmm0);
  ConvertNotNaNDoubleToJSVal(asm_->rax, asm_->rcx);
  asm_->jmp(".ARITHMETIC_EXIT");

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_MULTIPLY);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_ADD(const Instruction* instr) {
  const int16_t dst = Reg(instr[1].i16[0]);
  const int16_t lhs = Reg(instr[1].i16[1]);
  const int16_t rhs = Reg(instr[1].i16[2]);

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

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() || rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_ADD);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(asm_->rax, rhs);
    Int32Guard(rhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->add(asm_->eax, lhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->add(asm_->eax, rhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->add(asm_->eax, asm_->edx);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  // lhs and rhs is always int32 (but overflow)
  asm_->L(".ARITHMETIC_OVERFLOW");
  LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
  asm_->movsxd(asm_->rax, asm_->eax);
  asm_->movsxd(asm_->rdx, asm_->edx);
  asm_->add(asm_->rax, asm_->rdx);
  asm_->cvtsi2sd(asm_->xmm0, asm_->rax);
  asm_->movq(asm_->rax, asm_->xmm0);
  ConvertNotNaNDoubleToJSVal(asm_->rax, asm_->rcx);
  asm_->jmp(".ARITHMETIC_EXIT");

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_ADD);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_LSHIFT(const Instruction* instr) {
  const int16_t dst = Reg(instr[1].i16[0]);
  const int16_t lhs = Reg(instr[1].i16[1]);
  const int16_t rhs = Reg(instr[1].i16[2]);

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

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() || rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_LSHIFT);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->sal(asm_->eax, rhs_value & 0x1F);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rcx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rdx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rcx, asm_->rdx, ".ARITHMETIC_GENERIC");
    asm_->sal(asm_->eax, asm_->cl);
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_LSHIFT);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_RSHIFT(const Instruction* instr) {
  const int16_t dst = Reg(instr[1].i16[0]);
  const int16_t lhs = Reg(instr[1].i16[1]);
  const int16_t rhs = Reg(instr[1].i16[2]);

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

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() || rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_RSHIFT);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->sar(asm_->eax, rhs_value & 0x1F);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rcx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rdx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rcx, asm_->rdx, ".ARITHMETIC_GENERIC");
    asm_->sar(asm_->eax, asm_->cl);
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_RSHIFT);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

inline void Compiler::EmitBINARY_RSHIFT_LOGICAL(const Instruction* instr) {
  const int16_t dst = Reg(instr[1].i16[0]);
  const int16_t lhs = Reg(instr[1].i16[1]);
  const int16_t rhs = Reg(instr[1].i16[2]);

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

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() || rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->shr(asm_->eax, rhs_value & 0x1F);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rcx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rdx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rcx, asm_->rdx, ".ARITHMETIC_GENERIC");
    asm_->shr(asm_->eax, asm_->cl);
  }
  asm_->cmp(asm_->eax, 0);
  asm_->jl(".ARITHMETIC_DOUBLE");  // uint32_t

  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_DOUBLE");
  asm_->cvtsi2sd(asm_->xmm0, asm_->rax);
  asm_->movq(asm_->rax, asm_->xmm0);
  ConvertNotNaNDoubleToJSVal(asm_->rax, asm_->rcx);
  asm_->jmp(".ARITHMETIC_EXIT");

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
