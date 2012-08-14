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
class StoredSlot;

namespace railgun {

struct Instruction {
  Instruction(uint32_t arg) { u32[0] = arg; }  // NOLINT

  Instruction(RegisterID reg) {  // NOLINT
    assert(reg);
    i32[0] = reg->register_offset();
  }

  union {
    const void* label;  // use for direct threading
    uint32_t u32[2];
    int32_t i32[2];
    ptrdiff_t diff;
    int16_t i16[4];
    uint16_t u16[4];
    struct {
      int16_t i16[2];
      uint32_t u32;
    } ssw;
    struct {
      int16_t i16[2];
      int32_t to;
    } jump;
    Map* map;
    Chain* chain;
    StoredSlot* slot;
    uint64_t u64;
  };

  static Instruction UInt64(uint64_t value) {
    Instruction instr(0u);
    instr.u64 = value;
    return instr;
  }

  static Instruction Slot(StoredSlot* slot) {
    Instruction instr(0u);
    instr.slot = slot;
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

  static Instruction UInt32(uint32_t a, uint32_t b) {
    Instruction instr(0u);
    instr.u32[0] = a;
    instr.u32[1] = b;
    return instr;
  }

  static Instruction Int32(int32_t a, int32_t b) {
    Instruction instr(0u);
    instr.i32[0] = a;
    instr.i32[1] = b;
    return instr;
  }

  static Instruction SW(RegisterID a, uint32_t u32) {
    Instruction instr(0u);
    instr.ssw.i16[0] = static_cast<int16_t>(a->register_offset());
    instr.ssw.u32 = u32;
    return instr;
  }

  static Instruction SSW(RegisterID a, RegisterID b, uint32_t u32) {
    Instruction instr(0u);
    instr.ssw.i16[0] = static_cast<int16_t>(a->register_offset());
    instr.ssw.i16[1] = static_cast<int16_t>(b->register_offset());
    instr.ssw.u32 = u32;
    return instr;
  }

  static Instruction SSW(RegisterID a, int16_t b, uint32_t u32) {
    Instruction instr(0u);
    instr.ssw.i16[0] = static_cast<int16_t>(a->register_offset());
    instr.ssw.i16[1] = b;
    instr.ssw.u32 = u32;
    return instr;
  }

  static Instruction Jump(int32_t i32,
                          RegisterID a = RegisterID(), RegisterID b = RegisterID()) {
    Instruction instr(0u);
    if (a) {
      instr.jump.i16[0] = static_cast<int16_t>(a->register_offset());
    }
    if (b) {
      instr.jump.i16[1] = static_cast<int16_t>(b->register_offset());
    }
    instr.jump.to = i32;
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
