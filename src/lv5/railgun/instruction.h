#ifndef _IV_LV5_RAILGUN_INSTRUCTION_H_
#define _IV_LV5_RAILGUN_INSTRUCTION_H_
#include "debug.h"
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/instruction_fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

OP::Type Instruction::GetOP() const {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
  assert(VM::LabelTable().find(label) != VM::LabelTable().end());
  return VM::LabelTable().find(label)->second;
#else
  return static_cast<OP::Type>(value);
#endif
}

Instruction Instruction::GetOPInstruction(OP::Type op) {
  Instruction instr(op);
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
  instr.label = VM::DispatchTable()[op];
#endif
  return instr;
}


} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_INSTRUCTION_H_
