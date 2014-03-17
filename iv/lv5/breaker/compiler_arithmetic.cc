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
void Compiler::EmitBINARY_MULTIPLY(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Multiply(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);
  Xbyak::Label exit;

  if (!(lhs_type.IsNotInt32() || rhs_type.IsNotInt32())) {
    if (lhs_type.IsConstantInt32()) {
      const int32_t lhs_value = lhs_type.constant().int32();
      LoadVR(rax, rhs);
      Int32Guard(rhs, rax, ".DOUBLE");
      asm_->imul(eax, eax, lhs_value);
      asm_->jo(".OVERFLOW");
    } else if (rhs_type.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type.constant().int32();
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
    asm_->jmp(exit, Xbyak::CodeGenerator::T_NEAR);

    kill_last_used();

    // lhs and rhs are always int32 (but overflow)
    asm_->L(".OVERFLOW");
    LoadVRs(rax, lhs, rdx, rhs);
    asm_->cvtsi2sd(xmm0, eax);
    asm_->cvtsi2sd(xmm1, edx);
    asm_->mulsd(xmm0, xmm1);
    asm_->movq(rax, xmm0);
    ConvertDoubleToJSVal(rax);
    asm_->jmp(exit, Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".DOUBLE");
  LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
  LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);

  asm_->mulsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax, &exit);

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_MULTIPLY);

  asm_->L(exit);
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_DIVIDE(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Divide(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);
  Xbyak::Label exit;

  LoadDouble(lhs, xmm0, rsi, ".GENERIC");
  LoadDouble(rhs, xmm1, rsi, ".GENERIC");

  asm_->divsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax, &exit);

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_DIVIDE);

  asm_->L(exit);
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_ADD(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Add(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);
  Xbyak::Label exit;

  if (!(lhs_type.IsNotInt32() || rhs_type.IsNotInt32())) {
    if (lhs_type.IsConstantInt32()) {
      const int32_t lhs_value = lhs_type.constant().int32();
      LoadVR(rax, rhs);
      Int32Guard(rhs, rax, ".DOUBLE");
      asm_->add(eax, lhs_value);
      asm_->jo(".OVERFLOW");
    } else if (rhs_type.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type.constant().int32();
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
    asm_->jmp(exit, Xbyak::CodeGenerator::T_NEAR);

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
    asm_->jmp(exit, Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".DOUBLE");
  LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
  LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);

  asm_->addsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax, &exit);

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_ADD);

  asm_->L(exit);
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_MODULO(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Modulo(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
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
  if (lhs_type.IsNotInt32() ||
      rhs_type.IsNotInt32() ||
      (rhs_type.IsConstantInt32() && rhs_type.constant().int32() <= 0) ||
      (lhs_type.IsConstantInt32() && lhs_type.constant().int32() < 0)) {
    LoadVRs(rsi, lhs, rdx, rhs);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_MODULO);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  // Because LHS and RHS are not signed,
  // we should store 0 to edx
  if (lhs_type.IsConstantInt32()) {
    // RHS > 0
    const int32_t lhs_value = lhs_type.constant().int32();
    LoadVR(rcx, rhs);
    Int32Guard(rhs, rcx, ".GENERIC");
    asm_->test(ecx, ecx);
    asm_->jle(".GENERIC");
    asm_->xor(edx, edx);
    asm_->mov(eax, lhs_value);
  } else if (rhs_type.IsConstantInt32()) {
    // LHS >= 0
    const int32_t rhs_value = rhs_type.constant().int32();
    LoadVR(rax, lhs);
    Int32Guard(lhs, rax, ".GENERIC");
    asm_->test(eax, eax);
    asm_->js(".GENERIC");
    asm_->xor(edx, edx);
    asm_->mov(ecx, rhs_value);
  } else {
    LoadVRs(rax, lhs, rcx, rhs);
    Int32Guard(lhs, rax, ".GENERIC");
    Int32Guard(rhs, rcx, ".GENERIC");
    asm_->test(eax, eax);
    asm_->js(".GENERIC");
    asm_->test(ecx, ecx);
    asm_->jle(".GENERIC");
    asm_->xor(edx, edx);
  }
  asm_->idiv(ecx);
  asm_->mov(eax, edx);

  // boxing
  asm_->or(rax, r15);
  asm_->jmp(".EXIT");

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_MODULO);

  asm_->L(".EXIT");
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_SUBTRACT(const Instruction* instr) {
  const register_t dst = Reg(instr[1].i16[0]);
  const register_t lhs = Reg(instr[1].i16[1]);
  const register_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type = type_record_.Get(lhs);
  const TypeEntry rhs_type = type_record_.Get(rhs);
  const TypeEntry dst_type = TypeEntry::Subtract(lhs_type, rhs_type);

  // dst is constant
  if (dst_type.IsConstant()) {
    EmitConstantDest(dst_type, dst);
    type_record_.Put(dst, dst_type);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);
  Xbyak::Label exit;

  if (!(lhs_type.IsNotInt32() || rhs_type.IsNotInt32())) {
    if (rhs_type.IsConstantInt32()) {
      const int32_t rhs_value = rhs_type.constant().int32();
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
    asm_->jmp(exit, Xbyak::CodeGenerator::T_NEAR);

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
    asm_->jmp(exit, Xbyak::CodeGenerator::T_NEAR);
  }

  asm_->L(".DOUBLE");
  LoadDouble(lhs, xmm0, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);
  LoadDouble(rhs, xmm1, rsi, ".GENERIC", Xbyak::CodeGenerator::T_NEAR);

  asm_->subsd(xmm0, xmm1);
  BoxDouble(xmm0, xmm1, rax, &exit);

  kill_last_used();

  asm_->L(".GENERIC");
  LoadVRs(rsi, lhs, rdx, rhs);
  asm_->mov(rdi, r14);
  asm_->Call(&stub::BINARY_SUBTRACT);

  asm_->L(exit);
  asm_->mov(qword[r13 + dst * kJSValSize], rax);
  set_last_used_candidate(dst);
  type_record_.Put(dst, dst_type);
}

} } }  // namespace iv::lv5::breaker
