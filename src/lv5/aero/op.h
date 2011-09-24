// Aero operation codes
// highly inspired Irregexp in V8
// bytecode specification is jsregexp.cc in V8
#ifndef IV_LV5_AERO_OP_H_
#define IV_LV5_AERO_OP_H_
#include <cstddef>
#include "detail/array.h"
namespace iv {
namespace lv5 {
namespace aero {

#define IV_LV5_AERO_OPCODES(V)\
V(ASSERTION_BOL, 1)\
V(ASSERTION_EOL, 1)\
V(ASSERTION_WORD_BOUNDARY, 1)\
V(ASSERTION_WORD_BOUNDARY_INVERTED, 1)\
V(PUSH_CAPTURE, 5)\
V(PUSH_CAPTURES, 1)\
V(POP_CAPTURES, 1)\
V(PUSH_CURRENT_POSITION, 1)\
V(POP_CURRENT_POSITION, 1)\
V(PUSH_BACKTRACK, 5)\
V(CHECK_1BYTE_CHAR, 2)\
V(CHECK_2BYTE_CHAR, 3)\
V(CHECK_2CHAR_OR, 5)\
V(CHECK_RANGE, 5)/* variadic */\
V(CHECK_RANGE_INVERTED, 5)/* variadic */\
V(JUMP, 5)\
V(MATCH, 1)\

class OP {
 public:
  enum Type {
#define V(op, len) op,
IV_LV5_AERO_OPCODES(V)
#undef V
    NUM_OF_OPCODES
  };

  static inline const char* String(int op);
};

template<OP::Type op>
struct OPLength;

#define V(O, N)\
  template<>\
  struct OPLength<OP::O> {\
    static const int value = N;\
  };
IV_LV5_AERO_OPCODES(V)
#undef V

#define V(O, N) #O,
static const std::array<const char*, OP::NUM_OF_OPCODES + 1> kOPString = { {
  IV_LV5_AERO_OPCODES(V)
  "NUM_OF_OPCODES"
} };
#undef V

#define V(O, N) N,
static const std::array<std::size_t, OP::NUM_OF_OPCODES + 1> kOPLength = { {
  IV_LV5_AERO_OPCODES(V)
  1
} };
#undef V

const char* OP::String(int op) {
  assert(0 <= op && op < OP::NUM_OF_OPCODES);
  return kOPString[op];
}

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_OP_H_
