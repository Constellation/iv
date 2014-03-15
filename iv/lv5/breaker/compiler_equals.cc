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

static register_t FusedReg(const railgun::Instruction* instr,
                           railgun::OP::Type fused, int order) {
  return Compiler::Reg(
      (fused == railgun::OP::NOP) ?
      instr[1].i16[order + 1] : instr[1].jump.i16[order]);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_EQ(const Instruction* instr, OP::Type fused) {
  const register_t lhs = FusedReg(instr, fused, 0);
  const register_t rhs = FusedReg(instr, fused, 1);
  {
    const Assembler::LocalLabelScope scope(asm_);
    LoadVRs(rsi, lhs, rdx, rhs);
    Int32Guard(lhs, rsi, ".SLOW");
    Int32Guard(rhs, rdx, ".SLOW");
    asm_->cmp(esi, edx);

    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      if (fused == OP::IF_TRUE) {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->jmp(".EXIT");

      asm_->L(".SLOW");
      asm_->mov(rdi, r14);
      asm_->Call(&stub::BINARY_EQ);
      asm_->cmp(rax, Extract(JSTrue));
      if (fused == OP::IF_TRUE) {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->L(".EXIT");
      return;
    }

    const register_t dst = Reg(instr[1].i16[0]);
    asm_->sete(cl);
    ConvertBooleanToJSVal(cl, rax);
    asm_->jmp(".EXIT");

    asm_->L(".SLOW");
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_EQ);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(
        dst, TypeEntry::Equal(type_record_.Get(lhs), type_record_.Get(rhs)));
  }
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_STRICT_EQ(const Instruction* instr, OP::Type fused) {
  // CAUTION:(Constellation)
  // Because stub::BINARY_STRICT_EQ is not require Frame as first argument,
  // so register layout is different from BINARY_ other ops.
  // BINARY_STRICT_NE too.
  const register_t lhs = FusedReg(instr, fused, 0);
  const register_t rhs = FusedReg(instr, fused, 1);
  {
    const Assembler::LocalLabelScope scope(asm_);
    LoadVRs(rdi, lhs, rsi, rhs);
    Int32Guard(lhs, rdi, ".SLOW");
    Int32Guard(rhs, rsi, ".SLOW");
    asm_->cmp(esi, edi);

    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      if (fused == OP::IF_TRUE) {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->jmp(".EXIT");

      asm_->L(".SLOW");
      asm_->Call(&stub::BINARY_STRICT_EQ);
      asm_->cmp(rax, Extract(JSTrue));
      if (fused == OP::IF_TRUE) {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->L(".EXIT");
      return;
    }

    const register_t dst = Reg(instr[1].i16[0]);
    asm_->sete(cl);
    ConvertBooleanToJSVal(cl, rax);
    asm_->jmp(".EXIT");

    asm_->L(".SLOW");
    asm_->Call(&stub::BINARY_STRICT_EQ);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(
        dst,
        TypeEntry::StrictEqual(type_record_.Get(lhs), type_record_.Get(rhs)));
  }
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_NE(const Instruction* instr, OP::Type fused) {
  const register_t lhs = FusedReg(instr, fused, 0);
  const register_t rhs = FusedReg(instr, fused, 1);
  {
    const Assembler::LocalLabelScope scope(asm_);
    LoadVRs(rsi, lhs, rdx, rhs);
    Int32Guard(lhs, rsi, ".SLOW");
    Int32Guard(rhs, rdx, ".SLOW");
    asm_->cmp(esi, edx);

    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      if (fused == OP::IF_TRUE) {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->jmp(".EXIT");

      asm_->L(".SLOW");
      asm_->mov(rdi, r14);
      asm_->Call(&stub::BINARY_NE);
      asm_->cmp(rax, Extract(JSTrue));
      if (fused == OP::IF_TRUE) {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->L(".EXIT");
      return;
    }

    const register_t dst = Reg(instr[1].i16[0]);
    asm_->setne(cl);
    ConvertBooleanToJSVal(cl, rax);
    asm_->jmp(".EXIT");

    asm_->L(".SLOW");
    asm_->mov(rdi, r14);
    asm_->Call(&stub::BINARY_NE);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(
        dst,
        TypeEntry::NotEqual(type_record_.Get(lhs), type_record_.Get(rhs)));
  }
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_STRICT_NE(const Instruction* instr, OP::Type fused) {
  // CAUTION:(Constellation)
  // Because stub::BINARY_STRICT_EQ is not require Frame as first argument,
  // so register layout is different from BINARY_ other ops.
  // BINARY_STRICT_NE too.
  const register_t lhs = FusedReg(instr, fused, 0);
  const register_t rhs = FusedReg(instr, fused, 1);
  {
    const Assembler::LocalLabelScope scope(asm_);
    LoadVRs(rdi, lhs, rsi, rhs);
    Int32Guard(lhs, rdi, ".SLOW");
    Int32Guard(rhs, rsi, ".SLOW");
    asm_->cmp(esi, edi);

    if (fused != OP::NOP) {
      // fused jump opcode
      const std::string label = MakeLabel(instr);
      if (fused == OP::IF_TRUE) {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->jmp(".EXIT");

      asm_->L(".SLOW");
      asm_->Call(&stub::BINARY_STRICT_NE);
      asm_->cmp(rax, Extract(JSTrue));
      if (fused == OP::IF_TRUE) {
        asm_->je(label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        asm_->jne(label, Xbyak::CodeGenerator::T_NEAR);
      }
      asm_->L(".EXIT");
      return;
    }

    const register_t dst = Reg(instr[1].i16[0]);
    asm_->setne(cl);
    ConvertBooleanToJSVal(cl, rax);
    asm_->jmp(".EXIT");

    asm_->L(".SLOW");
    asm_->Call(&stub::BINARY_STRICT_NE);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(
        dst,
        TypeEntry::StrictNotEqual(
            type_record_.Get(lhs), type_record_.Get(rhs)));
  }
}

} } }  // namespace iv::lv5::breaker
