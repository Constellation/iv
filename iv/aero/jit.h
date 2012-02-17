#ifndef IV_AERO_JIT_H_
#define IV_AERO_JIT_H_
#include <vector>
#include <iv/detail/cstdint.h>
#include <iv/no_operator_names_guard.h>
#include <iv/assoc_vector.h>
#include <iv/aero/op.h>
#include <iv/aero/code.h>
#include <iv/aero/utility.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/xbyak/xbyak.h>
namespace iv {
namespace aero {

class JIT : public Xbyak::CodeGenerator {
 public:
  JIT() : targets_() { targets_.reserve(32); }

  void Compile(const Code& code) {
    std::vector<uint8_t>::const_pointer instr = code.bytes().data();
    const std::vector<uint8_t>::const_pointer last =
        code.bytes().data() + code.bytes().size();
    const std::vector<uint8_t>::const_pointer first_instr = instr;
    EmitPrologue();
    while (instr != last) {
      const uint8_t opcode = *instr;
      uint32_t length = kOPLength[opcode];
      if (opcode == OP::CHECK_RANGE || opcode == OP::CHECK_RANGE_INVERTED) {
        length += Load4Bytes(instr + 1);
      }
      switch (opcode) {
#define V(op, N)\
  case OP::op: {\
    RegisterJumpTarget(instr - first_instr);\
    Emit##op(instr, length);\
    break;\
  }
IV_AERO_OPCODES(V)
#undef V
      }
      std::advance(instr, length);
    }
    EmitEpilogue();
  }

  void EmitPrologue() {
  }

  void EmitEpilogue() {
  }

  void EmitSTORE_SP(const uint8_t* instr, uint32_t len) {
  }

  void EmitSTORE_POSITION(const uint8_t* instr, uint32_t len) {
  }

  void EmitPOSITION_TEST(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_SUCCESS(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_FAILURE(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_BOL(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_BOB(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_EOL(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_EOB(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_WORD_BOUNDARY(const uint8_t* instr, uint32_t len) {
  }

  void EmitASSERTION_WORD_BOUNDARY_INVERTED(const uint8_t* instr, uint32_t len) {
  }

  void EmitSTART_CAPTURE(const uint8_t* instr, uint32_t len) {
  }

  void EmitEND_CAPTURE(const uint8_t* instr, uint32_t len) {
  }

  void EmitCLEAR_CAPTURES(const uint8_t* instr, uint32_t len) {
  }

  void EmitCOUNTER_ZERO(const uint8_t* instr, uint32_t len) {
  }

  void EmitCOUNTER_NEXT(const uint8_t* instr, uint32_t len) {
  }

  void EmitPUSH_BACKTRACK(const uint8_t* instr, uint32_t len) {
  }

  void EmitBACK_REFERENCE(const uint8_t* instr, uint32_t len) {
  }

  void EmitBACK_REFERENCE_IGNORE_CASE(const uint8_t* instr, uint32_t len) {
  }

  void EmitCHECK_1BYTE_CHAR(const uint8_t* instr, uint32_t len) {
  }

  void EmitCHECK_2BYTE_CHAR(const uint8_t* instr, uint32_t len) {
  }

  void EmitCHECK_2CHAR_OR(const uint8_t* instr, uint32_t len) {
  }

  void EmitCHECK_3CHAR_OR(const uint8_t* instr, uint32_t len) {
  }

  void EmitCHECK_RANGE(const uint8_t* instr, uint32_t len) {
    for (uint32_t first = 1; first < len; ++first) {
    }
  }

  void EmitCHECK_RANGE_INVERTED(const uint8_t* instr, uint32_t len) {
    for (uint32_t first = 1; first < len; ++first) {
    }
  }

  void EmitJUMP(const uint8_t* instr, uint32_t len) {
  }

  void EmitFAILURE(const uint8_t* instr, uint32_t len) {
  }

  void EmitSUCCESS(const uint8_t* instr, uint32_t len) {
  }

  void RegisterJumpTarget(std::size_t offset) {
    targets_.insert(std::make_pair(offset, 0));
  }

  uintptr_t SearchAddress(std::size_t offset) {
    return targets_.find(offset)->second;
  }

 private:
  core::AssocVector<std::size_t, uintptr_t> targets_;
};

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_H_
