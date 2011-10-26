#ifndef IV_LV5_RAILGUN_OP_H_
#define IV_LV5_RAILGUN_OP_H_
#include <cassert>
#include "detail/cstdint.h"
#include "detail/array.h"
#include "detail/unordered_map.h"
#include "static_assert.h"
#include "lv5/symbol.h"
#include "lv5/railgun/direct_threading.h"
namespace iv {
namespace lv5 {

class Map;

namespace railgun {

// opcode name and length
#define IV_LV5_RAILGUN_OP_LIST(V)\
V(STOP_CODE, 1)\
V(NOP, 1)\
\
V(POP_TOP, 1)\
V(POP_TOP_AND_RET, 1)\
V(ROT_TWO, 1)\
V(ROT_THREE, 1)\
V(ROT_FOUR, 1)\
V(DUP_TOP, 1)\
V(DUP_TWO, 1)\
\
V(UNARY_POSITIVE, 1)\
V(UNARY_NEGATIVE, 1)\
V(UNARY_NOT, 1)\
V(UNARY_BIT_NOT, 1)\
\
V(BINARY_ADD, 1)\
V(BINARY_SUBTRACT, 1)\
V(BINARY_MULTIPLY, 1)\
V(BINARY_DIVIDE, 1)\
V(BINARY_MODULO, 1)\
V(BINARY_LSHIFT, 1)\
V(BINARY_RSHIFT, 1)\
V(BINARY_RSHIFT_LOGICAL, 1)\
V(BINARY_LT, 1)\
V(BINARY_LTE, 1)\
V(BINARY_GT, 1)\
V(BINARY_GTE, 1)\
V(BINARY_INSTANCEOF, 1)\
V(BINARY_IN, 1)\
V(BINARY_EQ, 1)\
V(BINARY_STRICT_EQ, 1)\
V(BINARY_NE, 1)\
V(BINARY_STRICT_NE, 1)\
V(BINARY_BIT_AND, 1)\
V(BINARY_BIT_XOR, 1)\
V(BINARY_BIT_OR, 1)\
\
V(RETURN, 1)\
V(THROW, 1)\
V(POP_ENV, 1)\
V(WITH_SETUP, 1)\
V(RETURN_SUBROUTINE, 1)\
V(DEBUGGER, 1)\
\
V(PUSH_EMPTY, 1)\
V(PUSH_UNDEFINED, 1)\
V(PUSH_TRUE, 1)\
V(PUSH_FALSE, 1)\
V(PUSH_NULL, 1)\
V(PUSH_THIS, 1)\
\
V(BUILD_OBJECT, 2)\
V(BUILD_REGEXP, 1)\
\
V(LOAD_ELEMENT, 1)\
V(STORE_ELEMENT, 1)\
V(DELETE_ELEMENT, 1)\
V(CALL_ELEMENT, 1)\
V(INCREMENT_ELEMENT, 1)\
V(DECREMENT_ELEMENT, 1)\
V(POSTFIX_INCREMENT_ELEMENT, 1)\
V(POSTFIX_DECREMENT_ELEMENT, 1)\
\
V(STORE_CALL_RESULT, 1)\
V(DELETE_CALL_RESULT, 1)\
V(CALL_CALL_RESULT, 1)\
V(INCREMENT_CALL_RESULT, 1)\
V(DECREMENT_CALL_RESULT, 1)\
V(POSTFIX_INCREMENT_CALL_RESULT, 1)\
V(POSTFIX_DECREMENT_CALL_RESULT, 1)\
\
V(TYPEOF, 1)\
\
/* opcodes over this requres argument */\
V(POP_N, 2)\
\
V(STORE_OBJECT_DATA, 3)\
V(STORE_OBJECT_GET, 3)\
V(STORE_OBJECT_SET, 3)\
\
/* property lookup opcodes */\
V(LOAD_PROP, 6)\
V(STORE_PROP, 6)\
V(DELETE_PROP, 6)\
V(CALL_PROP, 6)\
V(INCREMENT_PROP, 6)\
V(DECREMENT_PROP, 6)\
V(POSTFIX_INCREMENT_PROP, 6)\
V(POSTFIX_DECREMENT_PROP, 6)\
\
V(LOAD_PROP_GENERIC, 6)\
V(LOAD_PROP_OWN, 6)\
V(LOAD_PROP_PROTO, 6)\
V(LOAD_PROP_CHAIN, 6)\
\
V(STORE_PROP_GENERIC, 6)\
\
V(CALL_PROP_GENERIC, 6)\
V(CALL_PROP_OWN, 6)\
V(CALL_PROP_PROTO, 6)\
V(CALL_PROP_CHAIN, 6)\
\
V(LOAD_CONST, 2)\
V(PUSH_UINT32, 2)\
V(PUSH_INT32, 2)\
\
V(JUMP_BY, 2)\
V(JUMP_SUBROUTINE, 2)\
V(JUMP_RETURN_HOOKED_SUBROUTINE, 2)\
V(JUMP_IF_FALSE_OR_POP, 2)\
V(JUMP_IF_TRUE_OR_POP, 2)\
V(POP_JUMP_IF_FALSE, 2)\
V(POP_JUMP_IF_TRUE, 2)\
\
V(FORIN_SETUP, 2)\
V(FORIN_ENUMERATE, 2)\
V(SWITCH_CASE, 2)\
V(SWITCH_DEFAULT, 2)\
V(TRY_CATCH_SETUP, 2)\
\
/* name lookup opcodes */\
V(LOAD_NAME, 4)\
V(STORE_NAME, 4)\
V(CALL_NAME, 4)\
V(DELETE_NAME, 4)\
V(INCREMENT_NAME, 4)\
V(DECREMENT_NAME, 4)\
V(POSTFIX_INCREMENT_NAME, 4)\
V(POSTFIX_DECREMENT_NAME, 4)\
V(TYPEOF_NAME, 4)\
\
V(LOAD_LOCAL, 4)\
V(STORE_LOCAL, 4)\
V(CALL_LOCAL, 4)\
V(DELETE_LOCAL, 4)\
V(INCREMENT_LOCAL, 4)\
V(DECREMENT_LOCAL, 4)\
V(POSTFIX_INCREMENT_LOCAL, 4)\
V(POSTFIX_DECREMENT_LOCAL, 4)\
V(TYPEOF_LOCAL, 4)\
\
V(LOAD_GLOBAL, 4)\
V(STORE_GLOBAL, 4)\
V(CALL_GLOBAL, 4)\
V(DELETE_GLOBAL, 4)\
V(INCREMENT_GLOBAL, 4)\
V(DECREMENT_GLOBAL, 4)\
V(POSTFIX_INCREMENT_GLOBAL, 4)\
V(POSTFIX_DECREMENT_GLOBAL, 4)\
V(TYPEOF_GLOBAL, 4)\
\
V(LOAD_HEAP, 4)\
V(STORE_HEAP, 4)\
V(CALL_HEAP, 4)\
V(DELETE_HEAP, 4)\
V(INCREMENT_HEAP, 4)\
V(DECREMENT_HEAP, 4)\
V(POSTFIX_INCREMENT_HEAP, 4)\
V(POSTFIX_DECREMENT_HEAP, 4)\
V(TYPEOF_HEAP, 4)\
\
V(STORE_LOCAL_IMMUTABLE, 4)\
V(INCREMENT_LOCAL_IMMUTABLE, 4)\
V(DECREMENT_LOCAL_IMMUTABLE, 4)\
V(POSTFIX_INCREMENT_LOCAL_IMMUTABLE, 4)\
V(POSTFIX_DECREMENT_LOCAL_IMMUTABLE, 4)\
V(RAISE_IMMUTABLE, 4)\
\
V(CALL, 2)\
V(CONSTRUCT, 2)\
V(EVAL, 2)\
\
V(BUILD_FUNCTION, 2)\
\
V(INIT_VECTOR_ARRAY_ELEMENT, 2)\
V(INIT_SPARSE_ARRAY_ELEMENT, 2)\
V(BUILD_ARRAY, 2)

struct ERR {
  enum Type {
    NOT_REFERENCE = 0,
    NUM_OF_ERR
  };
};

struct OP {
#define IV_LV5_RAILGUN_DEFINE_ENUM(V, N) V,
  enum Type {
    IV_LV5_RAILGUN_OP_LIST(IV_LV5_RAILGUN_DEFINE_ENUM)
    NUM_OF_OP
  };
#undef IV_LV5_RAILGUN_DEFINE_ENUM

  IV_STATIC_ASSERT(NUM_OF_OP <= 256);

#define IS_NAME_LOOKUP_OP(op)\
  ( (op) == OP::LOAD_NAME ||\
    (op) == OP::STORE_NAME ||\
    (op) == OP::DELETE_NAME ||\
    (op) == OP::CALL_NAME ||\
    (op) == OP::INCREMENT_NAME ||\
    (op) == OP::DECREMENT_NAME ||\
    (op) == OP::POSTFIX_INCREMENT_NAME ||\
    (op) == OP::POSTFIX_DECREMENT_NAME ||\
    (op) == OP::TYPEOF_NAME )

  template<OP::Type op>
  struct IsNameLookupOP {
    static const bool value = IS_NAME_LOOKUP_OP(op);
  };

  static bool IsNameLookup(OP::Type op) {
    return IS_NAME_LOOKUP_OP(op);
  }

#undef IS_NAME_LOOKUP_OP

  static OP::Type ToGlobal(uint8_t op) {
    assert(IsNameLookup(static_cast<OP::Type>(op)));
    return static_cast<OP::Type>(op + (OP::LOAD_GLOBAL - OP::LOAD_NAME));
  }

  static OP::Type ToLocal(uint8_t op) {
    assert(IsNameLookup(static_cast<OP::Type>(op)));
    return static_cast<OP::Type>(op + (OP::LOAD_LOCAL - OP::LOAD_NAME));
  }

  static OP::Type ToHeap(uint8_t op) {
    assert(IsNameLookup(static_cast<OP::Type>(op)));
    return static_cast<OP::Type>(op + (OP::LOAD_HEAP - OP::LOAD_NAME));
  }

  static OP::Type ToLocalImmutable(uint8_t op, bool strict) {
    assert(IsNameLookup(static_cast<OP::Type>(op)));
    if (op == STORE_NAME) {
      if (strict) {
        return RAISE_IMMUTABLE;
      } else {
        return STORE_LOCAL_IMMUTABLE;
      }
    }
    if (op == INCREMENT_NAME) {
      if (strict) {
        return RAISE_IMMUTABLE;
      } else {
        return INCREMENT_LOCAL_IMMUTABLE;
      }
    }
    if (op == DECREMENT_NAME) {
      if (strict) {
        return RAISE_IMMUTABLE;
      } else {
        return DECREMENT_LOCAL_IMMUTABLE;
      }
    }
    if (op == POSTFIX_INCREMENT_NAME) {
      if (strict) {
        return RAISE_IMMUTABLE;
      } else {
        return POSTFIX_INCREMENT_LOCAL_IMMUTABLE;
      }
    }
    if (op == POSTFIX_DECREMENT_NAME) {
      if (strict) {
        return RAISE_IMMUTABLE;
      } else {
        return POSTFIX_DECREMENT_LOCAL_IMMUTABLE;
      }
    }
    return ToLocal(op);
  }

  static inline const char* String(int op);
};

#define IV_LV5_RAILGUN_DEFINE_STRINGS(V, N) #V,
static const std::array<const char*, OP::NUM_OF_OP + 1> kOPString = { {
  IV_LV5_RAILGUN_OP_LIST(IV_LV5_RAILGUN_DEFINE_STRINGS)
  "NUM_OF_OP"
} };
#undef IV_LV5_RAILGUN_DEFINE_STRINGS

#define IV_LV5_RAILGUN_DEFINE_LENGTH(V, N) N,
static const std::array<std::size_t, OP::NUM_OF_OP + 1> kOPLength = { {
  IV_LV5_RAILGUN_OP_LIST(IV_LV5_RAILGUN_DEFINE_LENGTH)
  1
} };
#undef IV_LV5_RAILGUN_DEFINE_LENGTH

template<OP::Type op>
struct OPLength;

#define IV_LV5_RAILGUN_DEFINE_LENGTH(V, N)\
  template<>\
  struct OPLength<OP::V> {\
    static const int value = N;\
  };
IV_LV5_RAILGUN_OP_LIST(IV_LV5_RAILGUN_DEFINE_LENGTH)
#undef IV_LV5_RAILGUN_DEFINE_LENGTH

const char* OP::String(int op) {
  assert(0 <= op && op < NUM_OF_OP);
  return kOPString[op];
}

typedef std::array<const void*, OP::NUM_OF_OP + 1> DirectThreadingDispatchTable;
typedef std::unordered_map<const void*, OP::Type> LabelOPTable;

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_OP_H_
