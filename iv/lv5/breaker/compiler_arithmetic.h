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

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
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

  // lhs and rhs are always int32 (but overflow)
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

  // lhs or rhs are not int32_t
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
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

  // lhs and rhs are always int32 (but overflow)
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32() ||
      (rhs_type_entry.IsConstantInt32() &&
       rhs_type_entry.constant().int32() <= 0) ||
      (lhs_type_entry.IsConstantInt32() &&
       lhs_type_entry.constant().int32() < 0)) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_MODULO);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
    LoadVR(asm_->rcx, rhs);
    Int32Guard(rhs, asm_->rcx, asm_->rdx, ".ARITHMETIC_GENERIC");
    asm_->cmp(asm_->ecx, 0);
    asm_->jle(".ARITHMETIC_GENERIC");
    asm_->xor(asm_->edx, asm_->edx);
    asm_->mov(asm_->eax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    // LHS >= 0
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->cmp(asm_->eax, 0);
    asm_->jl(".ARITHMETIC_GENERIC");
    asm_->xor(asm_->edx, asm_->edx);
    asm_->mov(asm_->ecx, rhs_value);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rcx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rdx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rcx, asm_->rdx, ".ARITHMETIC_GENERIC");
    asm_->cmp(asm_->eax, 0);
    asm_->jl(".ARITHMETIC_GENERIC");
    asm_->cmp(asm_->ecx, 0);
    asm_->jle(".ARITHMETIC_GENERIC");
    asm_->xor(asm_->edx, asm_->edx);
  }
  asm_->idiv(asm_->ecx);
  asm_->mov(asm_->eax, asm_->edx);

  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_MODULO);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
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

  // lhs or rhs are not int32_t
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_SUBTRACT);
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
    asm_->sub(asm_->eax, rhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->sub(asm_->eax, asm_->edx);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  // lhs and rhs are always int32 (but overflow)
  // So we just sub as int64_t and convert to double,
  // because INT32_MIN - INT32_MIN is in int64_t range, and convert to
  // double makes no error.
  asm_->L(".ARITHMETIC_OVERFLOW");
  LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
  asm_->movsxd(asm_->rax, asm_->eax);
  asm_->movsxd(asm_->rdx, asm_->edx);
  asm_->sub(asm_->rax, asm_->rdx);
  asm_->cvtsi2sd(asm_->xmm0, asm_->rax);
  asm_->movq(asm_->rax, asm_->xmm0);
  ConvertNotNaNDoubleToJSVal(asm_->rax, asm_->rcx);
  asm_->jmp(".ARITHMETIC_EXIT");

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_SUBTRACT);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_BIT_AND);
    if (fused != OP::NOP) {
      const std::string label = MakeLabel(instr);
      asm_->test(asm_->eax, asm_->eax);
      if (fused == OP::IF_TRUE) {
        asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, dst_type_entry);
    }
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(asm_->rax, rhs);
    Int32Guard(rhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->and(asm_->eax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->and(asm_->eax, rhs_value);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->and(asm_->eax, asm_->edx);
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
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_BIT_AND);

    asm_->test(asm_->eax, asm_->eax);
    if (fused == OP::IF_TRUE) {
      asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    } else {
      asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    }
    asm_->L(".ARITHMETIC_EXIT");
  } else {
    // boxing
    asm_->or(asm_->rax, asm_->r15);
    asm_->jmp(".ARITHMETIC_EXIT");

    kill_last_used();

    asm_->L(".ARITHMETIC_GENERIC");
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_BIT_AND);

    asm_->L(".ARITHMETIC_EXIT");
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_BIT_XOR);
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
    asm_->xor(asm_->eax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->xor(asm_->eax, rhs_value);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->xor(asm_->eax, asm_->edx);
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_BIT_XOR);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
  if (lhs_type_entry.type().IsNotInt32() ||
      rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_BIT_OR);
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
    asm_->or(asm_->rax, lhs_value);
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->or(asm_->rax, rhs_value);
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->or(asm_->rax, asm_->rdx);
  }
  // boxing, but because of 'or', we can remove boxing or phase.
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used();

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_BIT_OR);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type_entry);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
