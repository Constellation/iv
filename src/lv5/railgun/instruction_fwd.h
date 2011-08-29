#ifndef _IV_LV5_RAILGUN_INSTRUCTION_FWD_H_
#define _IV_LV5_RAILGUN_INSTRUCTION_FWD_H_
#include "lv5/symbol.h"
#include "lv5/railgun/op.h"
namespace iv {
namespace lv5 {
namespace railgun {

struct Instruction {
  Instruction(uint32_t arg) : value(arg) { }  // NOLINT
  union {
    const void* label;  // use for direct threading
    uint32_t value;
    int32_t i32;
    Map* map;
    StringSymbol symbol;
  };

  inline OP::Type GetOP() const;

  static inline Instruction GetOPInstruction(OP::Type op);

  std::size_t GetLength() const {
    return kOPLength[GetOP()];
  }
};

} } }  // namespace iv::lv5::railgun

// global scope
GC_DECLARE_PTRFREE(iv::lv5::railgun::Instruction);

#endif  // _IV_LV5_RAILGUN_INSTRUCTION_FWD_H_
