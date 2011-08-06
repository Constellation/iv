#ifndef _IV_LV5_RAILGUN_CORE_DATA_H_
#define _IV_LV5_RAILGUN_CORE_DATA_H_
#include "lv5/railgun/core_data_fwd.h"
#include "lv5/railgun/vm_fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

GC_ms_entry* CoreData::MarkChildren(GC_word* top,
                                    GC_ms_entry* entry,
                                    GC_ms_entry* mark_sp_limit,
                                    GC_word env) {
  entry = GC_MARK_AND_PUSH(data_,
                           entry, mark_sp_limit, reinterpret_cast<void**>(this));
  entry = GC_MARK_AND_PUSH(targets_,
                           entry, mark_sp_limit, reinterpret_cast<void**>(this));
  for (InstTargets::const_iterator it = targets_->begin(),
       last = targets_->end(); it != last; ++it) {
    // loop and search Map pointer operations
    // check global opcodes
    if (VM::IsOP<OP::LOAD_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::STORE_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::DELETE_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::CALL_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::INCREMENT_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::DECREMENT_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::POSTFIX_INCREMENT_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::POSTFIX_DECREMENT_GLOBAL>(**it)) {
    } else if (VM::IsOP<OP::TYPEOF_GLOBAL>(**it)) {
    }
  }
  return entry;
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CORE_DATA_H_
