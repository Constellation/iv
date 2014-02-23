// Aero operation codes
// highly inspired Irregexp in V8
// bytecode specification is jsregexp.cc in V8
#ifndef IV_AERO_OP_H_
#define IV_AERO_OP_H_
#include <cstddef>
#include <iv/detail/array.h>
#include <iv/aero/utility.h>
namespace iv {
namespace aero {

#define IV_AERO_OPCODES(V)\
V(STORE_POSITION, 5)\
V(POSITION_TEST, 5)\
V(ASSERTION_SUCCESS, 9)\
V(ASSERTION_FAILURE, 5)\
V(ASSERTION_BOL, 1)\
V(ASSERTION_BOB, 1)\
V(ASSERTION_EOL, 1)\
V(ASSERTION_EOB, 1)\
V(ASSERTION_WORD_BOUNDARY, 1)\
V(ASSERTION_WORD_BOUNDARY_INVERTED, 1)\
V(START_CAPTURE, 5)\
V(END_CAPTURE, 5)\
V(CLEAR_CAPTURES, 9)\
V(COUNTER_NEXT, 13)\
V(PUSH_BACKTRACK, 5)\
V(BACK_REFERENCE, 3)\
V(BACK_REFERENCE_IGNORE_CASE, 3)\
V(JUMP, 5)\
V(FAILURE, 1)\
V(SUCCESS, 1)\
/* below opcode can be optimized */\
V(STORE_SP, 5)\
V(COUNTER_ZERO, 5)\
V(CHECK_1BYTE_CHAR, 2)\
V(CHECK_2BYTE_CHAR, 3)\
V(CHECK_2CHAR_OR, 5)\
V(CHECK_3CHAR_OR, 7)\
V(CHECK_4CHAR_OR, 9)\
V(CHECK_RANGE, 7)/* variadic */\
V(CHECK_RANGE_INVERTED, 7)/* variadic */

class OP {
 public:
  enum Type {
#define V(op, len) op,
IV_AERO_OPCODES(V)
#undef V
    NUM_OF_OPCODES
  };

  template<typename Iter>
  static uint32_t GetLength(Iter instr);
  static inline const char* String(int op);
};

template<OP::Type op>
struct OPLength;

#define V(O, N)\
  template<>\
  struct OPLength<OP::O> {\
    static const int value = N;\
  };
IV_AERO_OPCODES(V)
#undef V

#define V(O, N) #O,
static const std::array<const char*, OP::NUM_OF_OPCODES + 1> kOPString = { {
  IV_AERO_OPCODES(V)
  "NUM_OF_OPCODES"
} };
#undef V

#define V(O, N) N,
static const std::array<std::size_t, OP::NUM_OF_OPCODES + 1> kOPLength = { {
  IV_AERO_OPCODES(V)
  1
} };
#undef V

const char* OP::String(int op) {
  assert(0 <= op && op < OP::NUM_OF_OPCODES);
  return kOPString[op];
}

template<typename Iter>
inline uint32_t OP::GetLength(Iter instr) {
  const uint8_t opcode = *instr;
  const uint32_t length = kOPLength[opcode];
  if (opcode == OP::CHECK_RANGE || opcode == OP::CHECK_RANGE_INVERTED) {
    return length + Load4Bytes(instr + 1);
  }
  return length;
}

} }  // namespace iv::aero
#endif  // IV_AERO_OP_H_
