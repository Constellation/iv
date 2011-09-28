#ifndef IV_LV5_AERO_VM_H_
#define IV_LV5_AERO_VM_H_
#include <vector>
#include <new>
#include "noncopyable.h"
#include "lv5/aero/code.h"
#include "lv5/aero/utility.h"
namespace iv {
namespace lv5 {
namespace aero {

class VM : private core::Noncopyable<VM> {
 public:
  VM() : backtrack_stack_(10000) { }
  void Execute(Code* code, int* captures);

 private:
  std::vector<int> backtrack_stack_;
};

#define DEFINE_OPCODE(op)\
  case op:
#define NEXT(op)\
  do {\
    instr += OPLength<op>::value;\
    continue;\
  } while (0)
#define BACKTRACK(op) break
#define PUSH(target)\
  do {\
    if (sp == stack_limit) {\
      return false;\
    }\
    *sp++ = target;\
  } while (0)
inline bool VM::Execute(Code* code, int* captures) {
  int* backtrack_base = backtrack_stack_.data();
  int* const backtrack_limit =
      backtrack_stack_.data() + backtrack_stack_.size();
  int* backtrack_sp = backtrack_base;
  const uint8_t* instr = code->data();
  for (;;) {
    // fetch opcode
    switch (instr[0]) {
      DEFINE_OPCODE(CHECK_1BYTE_CHAR) {
        if (current == Load1Bytes(instr + 1)) {
          ++current_position;
          NEXT(CHECK_1BYTE_CHAR);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_2BYTE_CHAR) {
        if (current == Load2Bytes(instr + 1)) {
          ++current_position;
          NEXT(CHECK_2BYTE_CHAR);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_BOL) {
        if (IsBOL()) {
          NEXT(ASSERTION_BOL);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOL) {
        if (IsEOL()) {
          NEXT(ASSERTION_EOL);
        }
        BACKTRACK();
      }
    }
    // returns to backtracked path
  }
}
#undef DEFINE_OPCODE
#undef NEXT
#undef BACKTRACK
#undef PUSH

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_VM_H_
