#ifndef _IV_LV5_RAILGUN_INSTRUCTION_H_
#define _IV_LV5_RAILGUN_INSTRUCTION_H_
#include "lv5/railgun/vm_fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/instruction_fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

OP::Type Instruction::GetOP() const {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
  const DirectThreadingDispatchTable& table = VM::DispatchTable();
  std::size_t index = 0;
  for (DirectThreadingDispatchTable::const_iterator it = table.begin(),
       last = table.end(); it != last; ++it, ++index) {
    if (label == *it) {
      return static_cast<OP::Type>(index);
    }
  }
  UNREACHABLE();
  return OP::NUM_OF_OP;  // makes compiler happy
#else
  return static_cast<OP::Type>(value);
#endif
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_INSTRUCTION_H_
