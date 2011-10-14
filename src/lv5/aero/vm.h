#ifndef IV_LV5_AERO_VM_H_
#define IV_LV5_AERO_VM_H_
#include <vector>
#include <new>
#include "noncopyable.h"
#include "ustringpiece.h"
#include "scoped_ptr.h"
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

  VM()
    : backtrack_stack_(kStackSize),
      backtrack_base_(backtrack_stack_.data()) { }

  bool Execute(const core::UStringPiece& subject,
               Code* code, int* captures,
               std::size_t current_position);

 private:
  int* NewState(int* current, std::size_t size) {
    if ((current + size) <= backtrack_base_ + kStackSize) {
      return current + size;
    }
    // overflow
    return NULL;
  }

  std::vector<int> backtrack_stack_;
  int* backtrack_base_;
};

#define DEFINE_OPCODE(op)\
  case OP::op:
#define DISPATCH() continue
#define ADVANCE(len)\
  do {\
    instr += len;\
  } while (0);\
  DISPATCH()
#define DISPATCH_NEXT(op) ADVANCE(OPLength<OP::op>::value)
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
  // captures and counters and jump target
  const std::size_t size = code->captures() * 2 + code->counters() + 1;
  // state layout is following
  // [ captures ][ captures ][ target ]
  core::ScopedPtr<int[]> state(new int[size]);
  int* sp = backtrack_base_;
//  int* sp = NewState(backtrack_base, size);
//  if (!sp) {
//    return false;
//  }
  const uint8_t* instr = code->data();
  const uint8_t* const first_instr = instr;
  for (;;) {
    // fetch opcode
    switch (instr[0]) {
      DEFINE_OPCODE(PUSH_BACKTRACK) {
        int* target = sp;
        if ((sp = NewState(sp, size))) {
          // copy state and push to backtrack stack
          std::copy(state.get(), state.get() + size - 1, target);
          target[size - 1] = static_cast<int>(Load4Bytes(instr + 1));
          target[1] = current_position;
        } else {
          // stack overflowed
          return false;
        }
        DISPATCH_NEXT(PUSH_BACKTRACK);
      }

      DEFINE_OPCODE(DISCARD_BACKTRACK) {
        sp -= size;
        DISPATCH_NEXT(DISCARD_BACKTRACK);
      }

      DEFINE_OPCODE(COUNTER_ZERO) {
        state[Load4Bytes(instr + 1)] = 0;
        DISPATCH_NEXT(COUNTER_ZERO);
      }

      DEFINE_OPCODE(COUNTER_NEXT) {
        // COUNTER_NEXT | COUNTER_TARGET | MAX | JUMP_TARGET
        const int32_t max = static_cast<int32_t>(Load4Bytes(instr + 5));
        if (++state[Load4Bytes(instr + 1)] < max) {
          instr = first_instr + Load4Bytes(instr + 9);
          DISPATCH();
        }
        DISPATCH_NEXT(COUNTER_NEXT);
      }

      DEFINE_OPCODE(CHECK_1BYTE_CHAR) {
        if (current_position < subject.size() &&
            subject[current_position] == Load1Bytes(instr + 1)) {
          ++current_position;
          DISPATCH_NEXT(CHECK_1BYTE_CHAR);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_2BYTE_CHAR) {
        if (current_position < subject.size() &&
            subject[current_position] == Load2Bytes(instr + 1)) {
          ++current_position;
          DISPATCH_NEXT(CHECK_2BYTE_CHAR);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_2CHAR_OR) {
        if (current_position < subject.size()) {
          const uint16_t ch = subject[current_position];
          if (ch == Load2Bytes(instr + 1) || ch == Load2Bytes(instr + 3)) {
            ++current_position;
            DISPATCH_NEXT(CHECK_2CHAR_OR);
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_3CHAR_OR) {
        if (current_position < subject.size()) {
          const uint16_t ch = subject[current_position];
          if (ch == Load2Bytes(instr + 1) ||
              ch == Load2Bytes(instr + 3) ||
              ch == Load2Bytes(instr + 5)) {
            ++current_position;
            DISPATCH_NEXT(CHECK_3CHAR_OR);
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION) {
        int* previous = sp = sp - size;
        std::copy(previous, previous + size, state.get());
        instr = first_instr + Load4Bytes(instr + 1);
        DISPATCH();
      }

      DEFINE_OPCODE(ASSERTION_BOB) {
        if (current_position == 0) {
          DISPATCH_NEXT(ASSERTION_BOB);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOB) {
        if (current_position == subject.size()) {
          DISPATCH_NEXT(ASSERTION_EOB);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_BOL) {
        if (current_position == 0 ||
            core::character::IsLineTerminator(subject[current_position])) {
          DISPATCH_NEXT(ASSERTION_BOL);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOL) {
        if (current_position == subject.size() ||
            core::character::IsLineTerminator(subject[current_position])) {
          DISPATCH_NEXT(ASSERTION_EOL);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_WORD_BOUNDARY) {
        if (IsWordSeparatorPrev(subject, current_position) !=
            IsWordSeparator(subject, current_position)) {
          DISPATCH_NEXT(ASSERTION_WORD_BOUNDARY);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_WORD_BOUNDARY_INVERTED) {
        if (IsWordSeparatorPrev(subject, current_position) ==
            IsWordSeparator(subject, current_position)) {
          DISPATCH_NEXT(ASSERTION_WORD_BOUNDARY_INVERTED);
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_RANGE) {
        if (current_position < subject.size()) {
          const uint16_t ch = subject[current_position];
          const uint32_t length = Load4Bytes(instr + 1);
          bool in_range = false;
          for (std::size_t i = 0; i < length; ++i) {
            const uint16_t start = Load2Bytes(instr + 1 + 4 + i * 2);
            const uint16_t finish = Load2Bytes(instr + 1 + 4 + i * 2 + 1);
            if (finish < ch) {
              break;
            }
            if (start <= ch && ch <= finish) {
              in_range = true;
              break;
            }
          }
          if (in_range) {
            ADVANCE(length + OPLength<OP::CHECK_RANGE>::value);
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_RANGE_INVERTED) {
        BACKTRACK();
        if (current_position < subject.size()) {
          const uint16_t ch = subject[current_position];
          const uint32_t length = Load4Bytes(instr + 1);
          bool in_range = false;
          for (std::size_t i = 0; i < length; ++i) {
            const uint16_t start = Load2Bytes(instr + 1 + 4 + i * 2);
            const uint16_t finish = Load2Bytes(instr + 1 + 4 + i * 2 + 1);
            if (finish < ch) {
              break;
            }
            if (start <= ch && ch <= finish) {
              in_range = true;
              break;
            }
          }
          if (!in_range) {
            ADVANCE(length + OPLength<OP::CHECK_RANGE_INVERTED>::value);
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(FAILURE) {
        BACKTRACK();
      }

      DEFINE_OPCODE(SUCCESS) {
        return true;
      }

      DEFINE_OPCODE(JUMP) {
        instr = first_instr + Load4Bytes(instr + 1);
        DISPATCH();
      }
    }
    // backtrack stack unwind
    if (sp > backtrack_base_) {
      const int* previous = sp = sp - size;
      std::copy(previous, previous + size, state.get());
      current_position = state[1];
      instr = first_instr + state[size - 1];
      DISPATCH();
    }
    return false;
  }
  return true;  // makes compiler happy
}
#undef DEFINE_OPCODE
#undef DISPATCH
#undef ADVANCE
#undef DISPATCH_NEXT
#undef BACKTRACK
#undef PUSH

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_VM_H_
