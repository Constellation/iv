#ifndef IV_LV5_RAILGUN_OP_H_
#define IV_LV5_RAILGUN_OP_H_
#include <cassert>
#include <iv/detail/cstdint.h>
#include <iv/detail/array.h>
#include <iv/detail/unordered_map.h>
#include <iv/static_assert.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/railgun/direct_threading.h>
namespace iv {
namespace lv5 {

class Map;

namespace railgun {

// opcode name and length
#define IV_LV5_RAILGUN_OP_LIST(V)\
V(STOP_CODE, 1)\
V(NOP, 1)\
V(MV, 3)\
\
V(UNARY_POSITIVE, 3)\
V(UNARY_NEGATIVE, 3)\
V(UNARY_NOT, 3)\
V(UNARY_BIT_NOT, 3)\
\
V(BINARY_ADD, 4)\
V(BINARY_SUBTRACT, 4)\
V(BINARY_MULTIPLY, 4)\
V(BINARY_DIVIDE, 4)\
V(BINARY_MODULO, 4)\
V(BINARY_LSHIFT, 4)\
V(BINARY_RSHIFT, 4)\
V(BINARY_RSHIFT_LOGICAL, 4)\
V(BINARY_LT, 4)\
V(BINARY_LTE, 4)\
V(BINARY_GT, 4)\
V(BINARY_GTE, 4)\
V(BINARY_INSTANCEOF, 4)\
V(BINARY_IN, 4)\
V(BINARY_EQ, 4)\
V(BINARY_STRICT_EQ, 4)\
V(BINARY_NE, 4)\
V(BINARY_STRICT_NE, 4)\
V(BINARY_BIT_AND, 4)\
V(BINARY_BIT_XOR, 4)\
V(BINARY_BIT_OR, 4)\
\
V(RETURN, 2)\
V(THROW, 2)\
V(POP_ENV, 1)\
V(WITH_SETUP, 2)\
V(RETURN_SUBROUTINE, 2)\
V(DEBUGGER, 1)\
\
V(PUSH_UNDEFINED, 1)\
V(PUSH, 2)\
\
V(LOAD_UNDEFINED, 2)\
V(LOAD_TRUE, 2)\
V(LOAD_FALSE, 2)\
V(LOAD_NULL, 2)\
V(LOAD_THIS, 2)\
\
V(BUILD_OBJECT, 3)\
V(BUILD_REGEXP, 3)\
\
V(LOAD_ELEMENT, 4)\
V(STORE_ELEMENT, 4)\
V(DELETE_ELEMENT, 4)\
V(CALL_ELEMENT, 4)\
V(INCREMENT_ELEMENT, 4)\
V(DECREMENT_ELEMENT, 4)\
V(POSTFIX_INCREMENT_ELEMENT, 4)\
V(POSTFIX_DECREMENT_ELEMENT, 4)\
\
V(RAISE_REFERENCE, 1)\
V(INCREMENT_CALL_RESULT, 2)\
V(DECREMENT_CALL_RESULT, 2)\
V(POSTFIX_INCREMENT_CALL_RESULT, 2)\
V(POSTFIX_DECREMENT_CALL_RESULT, 2)\
\
V(TYPEOF, 3)\
\
V(STORE_OBJECT_DATA, 5)\
V(STORE_OBJECT_GET, 5)\
V(STORE_OBJECT_SET, 5)\
\
/* property lookup opcodes */\
V(LOAD_PROP, 8)\
V(STORE_PROP, 8)\
V(DELETE_PROP, 8)\
V(CALL_PROP, 8)\
V(INCREMENT_PROP, 8)\
V(DECREMENT_PROP, 8)\
V(POSTFIX_INCREMENT_PROP, 8)\
V(POSTFIX_DECREMENT_PROP, 8)\
\
V(LOAD_PROP_GENERIC, 8)\
V(LOAD_PROP_OWN, 8)\
V(LOAD_PROP_PROTO, 8)\
V(LOAD_PROP_CHAIN, 8)\
\
V(STORE_PROP_GENERIC, 8)\
\
V(CALL_PROP_GENERIC, 8)\
V(CALL_PROP_OWN, 8)\
V(CALL_PROP_PROTO, 8)\
V(CALL_PROP_CHAIN, 8)\
\
V(LOAD_CONST, 3)\
V(LOAD_UINT32, 3)\
V(LOAD_INT32, 3)\
\
V(JUMP_BY, 2)\
V(JUMP_SUBROUTINE, 3)\
V(IF_FALSE, 3)\
V(IF_TRUE, 3)\
\
V(FORIN_SETUP, 4)\
V(FORIN_ENUMERATE, 4)\
V(TRY_CATCH_SETUP, 3)\
\
/* name lookup opcodes */\
V(LOAD_NAME, 3)\
V(STORE_NAME, 3)\
V(CALL_NAME, 3)\
V(DELETE_NAME, 3)\
V(INCREMENT_NAME, 3)\
V(DECREMENT_NAME, 3)\
V(POSTFIX_INCREMENT_NAME, 3)\
V(POSTFIX_DECREMENT_NAME, 3)\
V(TYPEOF_NAME, 3)\
\
V(LOAD_LOCAL, 3)\
V(STORE_LOCAL, 3)\
V(CALL_LOCAL, 3)\
V(DELETE_LOCAL, 3)\
V(INCREMENT_LOCAL, 3)\
V(DECREMENT_LOCAL, 3)\
V(POSTFIX_INCREMENT_LOCAL, 3)\
V(POSTFIX_DECREMENT_LOCAL, 3)\
V(TYPEOF_LOCAL, 3)\
\
V(LOAD_GLOBAL, 5)\
V(STORE_GLOBAL, 5)\
V(CALL_GLOBAL, 5)\
V(DELETE_GLOBAL, 5)\
V(INCREMENT_GLOBAL, 5)\
V(DECREMENT_GLOBAL, 5)\
V(POSTFIX_INCREMENT_GLOBAL, 5)\
V(POSTFIX_DECREMENT_GLOBAL, 5)\
V(TYPEOF_GLOBAL, 5)\
\
V(LOAD_HEAP, 5)\
V(STORE_HEAP, 5)\
V(CALL_HEAP, 5)\
V(DELETE_HEAP, 5)\
V(INCREMENT_HEAP, 5)\
V(DECREMENT_HEAP, 5)\
V(POSTFIX_INCREMENT_HEAP, 5)\
V(POSTFIX_DECREMENT_HEAP, 5)\
V(TYPEOF_HEAP, 5)\
\
V(STORE_LOCAL_IMMUTABLE, 3)\
V(INCREMENT_LOCAL_IMMUTABLE, 3)\
V(DECREMENT_LOCAL_IMMUTABLE, 3)\
V(POSTFIX_INCREMENT_LOCAL_IMMUTABLE, 3)\
V(POSTFIX_DECREMENT_LOCAL_IMMUTABLE, 3)\
V(RAISE_IMMUTABLE, 2)\
V(RAISE_REFERENCE, 1)\
V(TO_NUMBER_AND_RAISE_REFERENCE, 1)\
\
V(CALL, 4)\
V(CONSTRUCT, 4)\
V(EVAL, 4)\
\
V(LOAD_FUNCTION, 3)\
\
V(INIT_VECTOR_ARRAY_ELEMENT, 4)\
V(INIT_SPARSE_ARRAY_ELEMENT, 4)\
V(BUILD_ARRAY, 3)\
\
/* binding instantiations */\
V(LOAD_PARAM, 3)\
V(LOAD_CALLEE, 2)\
V(BUILD_ENV, 2)\
V(INSTANTIATE_DECLARATION_BINDING, 3)\
V(INSTANTIATE_VARIABLE_BINDING, 3)\
V(INSTANTIATE_HEAP_BINDING, 4)\
V(INITIALIZE_HEAP_IMMUTABLE, 3)\
V(LOAD_ARGUMENTS, 2)

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
