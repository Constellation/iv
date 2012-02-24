#ifndef IV_LV5_RAILGUN_INSTRUCTION_H_
#define IV_LV5_RAILGUN_INSTRUCTION_H_
#include <iv/debug.h>
#include <iv/lv5/railgun/vm_fwd.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/instruction_fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

OP::Type Instruction::GetOP() const {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
  assert(VM::LabelTable().find(label) != VM::LabelTable().end());
  return VM::LabelTable().find(label)->second;
#else
  return static_cast<OP::Type>(u32[0]);
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
#endif  // IV_LV5_RAILGUN_INSTRUCTION_H_
