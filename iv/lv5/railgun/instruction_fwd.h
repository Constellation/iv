#ifndef IV_LV5_RAILGUN_INSTRUCTION_FWD_H_
#define IV_LV5_RAILGUN_INSTRUCTION_FWD_H_
#include <iv/debug.h>
#include <iv/static_assert.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/register_id.h>
namespace iv {
namespace lv5 {

class Map;
class Chain;

namespace railgun {

struct Instruction {
  Instruction(uint32_t arg) : u32(arg) { }  // NOLINT

  Instruction(RegisterID reg) {  // NOLINT
    assert(reg);
    i32 = reg->register_offset();
  }

  union {
    const void* label;  // use for direct threading
    uint32_t u32;
    int32_t i32;
    ptrdiff_t diff;
    int16_t i16[4];
    uint16_t u16[4];
    struct {
      union {
        uint16_t u16;
        int16_t i16;
      } v16[2];
      uint32_t u32;
    } ssw;
    Map* map;
    Chain* chain;
  };

  static Instruction Diff(ptrdiff_t to, ptrdiff_t from) {
    Instruction instr(0u);
    instr.diff = to - from;
    return instr;
  }

  static Instruction Reg2(RegisterID a, RegisterID b) {
    Instruction instr(0u);
    instr.i16[0] = static_cast<int16_t>(a->register_offset());
    instr.i16[1] = static_cast<int16_t>(b->register_offset());
    return instr;
  }

  static Instruction Reg3(RegisterID a, RegisterID b, RegisterID c) {
    Instruction instr(0u);
    instr.i16[0] = static_cast<int16_t>(a->register_offset());
    instr.i16[1] = static_cast<int16_t>(b->register_offset());
    instr.i16[2] = static_cast<int16_t>(c->register_offset());
    return instr;
  }

  static Instruction SSW(RegisterID a, RegisterID b, uint32_t u32) {
    Instruction instr(0u);
    if (a) {
      instr.ssw.v16[0].i16 = static_cast<int16_t>(a->register_offset());
    }
    if (b) {
      instr.ssw.v16[1].i16 = static_cast<int16_t>(a->register_offset());
    }
    instr.ssw.u32 = u32;
    return instr;
  }

  inline OP::Type GetOP() const;

  static inline Instruction GetOPInstruction(OP::Type op);

  std::size_t GetLength() const {
    return kOPLength[GetOP()];
  }
};

IV_STATIC_ASSERT(sizeof(Instruction) == 8); // always 64bit

} } }  // namespace iv::lv5::railgun

// global scope
GC_DECLARE_PTRFREE(iv::lv5::railgun::Instruction);

#endif  // IV_LV5_RAILGUN_INSTRUCTION_FWD_H_
