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
#include <iv/lv5/breaker/compiler_comparison.h>
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

  static void JumpInt(Assembler* assembler,
                      bool if_true, const Xbyak::Label* label) {
    if (if_true) {
      assembler->jl(*label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jge(*label, Xbyak::CodeGenerator::T_NEAR);
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
      const Xbyak::Label* label) {
    assembler->ucomisd(fp1, fp0);  // inverted
    if (label) {
      if (if_true) {
        assembler->ja(*label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jbe(*label, Xbyak::CodeGenerator::T_NEAR);
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

  static void JumpInt(Assembler* assembler,
                      bool if_true, const Xbyak::Label* label) {
    if (if_true) {
      assembler->jle(*label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jg(*label, Xbyak::CodeGenerator::T_NEAR);
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
      const Xbyak::Label* label = nullptr) {
    assembler->ucomisd(fp1, fp0);  // inverted
    if (label) {
      if (if_true) {
        assembler->jae(*label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jb(*label, Xbyak::CodeGenerator::T_NEAR);
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

  static void JumpInt(Assembler* assembler,
                      bool if_true, const Xbyak::Label* label) {
    if (if_true) {
      assembler->jg(*label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jle(*label, Xbyak::CodeGenerator::T_NEAR);
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
      const Xbyak::Label* label = nullptr) {
    assembler->ucomisd(fp0, fp1);
    if (label) {
      if (if_true) {
        assembler->ja(*label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jbe(*label, Xbyak::CodeGenerator::T_NEAR);
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

  static void JumpInt(Assembler* assembler,
                      bool if_true, const Xbyak::Label* label) {
    if (if_true) {
      assembler->jge(*label, Xbyak::CodeGenerator::T_NEAR);
    } else {
      assembler->jl(*label, Xbyak::CodeGenerator::T_NEAR);
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
      const Xbyak::Label* label = nullptr) {
    assembler->ucomisd(fp0, fp1);
    if (label) {
      if (if_true) {
        assembler->jae(*label, Xbyak::CodeGenerator::T_NEAR);
      } else {
        assembler->jb(*label, Xbyak::CodeGenerator::T_NEAR);
      }
    } else {
      assert(if_true == true);
      assembler->setae(reg);
    }
  }
};

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_LT(const Instruction* instr, OP::Type fused) {
  EmitCompare<LTTraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_LTE(const Instruction* instr, OP::Type fused) {
  EmitCompare<LTETraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_GT(const Instruction* instr, OP::Type fused) {
  EmitCompare<GTTraits>(instr, fused);
}

// opcode | (dst | lhs | rhs)
void Compiler::EmitBINARY_GTE(const Instruction* instr, OP::Type fused) {
  EmitCompare<GTETraits>(instr, fused);
}

} } }  // namespace iv::lv5::breaker
