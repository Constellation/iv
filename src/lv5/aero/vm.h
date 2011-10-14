#ifndef IV_LV5_AERO_VM_H_
#define IV_LV5_AERO_VM_H_
#include <vector>
#include <new>
#include "noncopyable.h"
#include "ustringpiece.h"
#include "lv5/aero/code.h"
#include "lv5/aero/character.h"
#include "lv5/aero/utility.h"
namespace iv {
namespace lv5 {
namespace aero {

inline bool IsWordSeparatorPrev(const core::UStringPiece& subject,
                                std::size_t current_position) {
  if (current_position == 0 || current_position > subject.size()) {
    return false;
  }
  return character::IsWord(subject[current_position - 1]);
}

inline bool IsWordSeparator(const core::UStringPiece& subject,
                            std::size_t current_position) {
  if (current_position >= subject.size()) {
    return false;
  }
  return character::IsWord(subject[current_position]);
}

class VM : private core::Noncopyable<VM> {
 public:
  static const std::size_t kStackSize = 10000;
  VM() : backtrack_stack_(kStackSize) { }
  bool Execute(const core::UStringPiece& subject,
               Code* code, int* captures,
               std::size_t current_position);

 private:
  std::vector<int> backtrack_stack_;
};

#define DEFINE_OPCODE(op)\
  case OP::op:
#define NEXT(op)\
  do {\
    instr += OPLength<OP::op>::value;\
    continue;\
  } while (0)
#define BACKTRACK() break;
#define PUSH(target)\
  do {\
    if (sp == stack_limit) {\
      return false;\
    }\
    *sp++ = target;\
  } while (0)
inline bool VM::Execute(const core::UStringPiece& subject,
                        Code* code, int* captures,
                        std::size_t current_position) {
  int* backtrack_base = backtrack_stack_.data();
  const uint8_t* instr = code->data();
  for (;;) {
    // fetch opcode
    switch (instr[0]) {
      DEFINE_OPCODE(CHECK_1BYTE_CHAR) {
        if (current_position < subject.size() &&
            subject[current_position] == Load1Bytes(instr + 1)) {
          ++current_position;
          NEXT(CHECK_1BYTE_CHAR);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_2BYTE_CHAR) {
        if (current_position < subject.size() &&
            subject[current_position] == Load2Bytes(instr + 1)) {
          ++current_position;
          NEXT(CHECK_2BYTE_CHAR);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_BOB) {
        if (current_position == 0) {
          NEXT(ASSERTION_BOB);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOB) {
        if (current_position == subject.size()) {
          NEXT(ASSERTION_EOB);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_BOL) {
        if (current_position == 0 ||
            core::character::IsLineTerminator(subject[current_position])) {
          NEXT(ASSERTION_BOL);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOL) {
        if (current_position == subject.size() ||
            core::character::IsLineTerminator(subject[current_position])) {
          NEXT(ASSERTION_EOL);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_WORD_BOUNDARY) {
        if (IsWordSeparatorPrev(subject, current_position) !=
            IsWordSeparator(subject, current_position)) {
          NEXT(ASSERTION_WORD_BOUNDARY);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_WORD_BOUNDARY_INVERTED) {
        if (IsWordSeparatorPrev(subject, current_position) ==
            IsWordSeparator(subject, current_position)) {
          NEXT(ASSERTION_WORD_BOUNDARY_INVERTED);
        }
        BACKTRACK();
      }

    }
    // backtrack loop 
  }
  return true;
}
#undef DEFINE_OPCODE
#undef NEXT
#undef BACKTRACK
#undef PUSH

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_VM_H_
