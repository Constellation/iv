#ifndef IV_LV5_AERO_VM_H_
#define IV_LV5_AERO_VM_H_
#include <vector>
#include "noncopyable.h"
#include "lv5/aero/code.h"
namespace iv {
namespace lv5 {
namespace aero {

class VM : private core::Noncopyable<VM> {
 public:
  VM() : backtrack_stack_(10000) { }
  inline void Execute(Code* code, int* captures);
 private:
  std::vector<int> backtrack_stack_;
};

#define DEFINE_OPCODE(op)\
    case op:
#define DISPATCH(op) continue
void VM::Execute(Code* code, int* captures) {
  int* backtrack_base = backtrack_stack_.data();
  int* const backtrack_limit =
      backtrack_stack_.data() + backtrack_stack_.size();
  int* backtrack_sp = backtrack_base;
  Opcode* instr = code->data();
  for (;;) {
    // fetch opcode
    switch (instr[0]) {
      DEFINE_OPCODE(LT) {
        DISPATCH(LT);
      }

      DEFINE_OPCODE(GT) {
        DISPATCH(GT);
      }
    }
    // failed
    // returns to backtracked path
  }
}
#undef DEFINE_OPCODE
#undef DISPATCH

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_VM_H_
