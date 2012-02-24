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
V(NOP, 1)\
V(MV, 2)\
\
V(UNARY_POSITIVE, 2)\
V(UNARY_NEGATIVE, 2)\
V(UNARY_NOT, 2)\
V(UNARY_BIT_NOT, 2)\
\
V(BINARY_ADD, 2)\
V(BINARY_SUBTRACT, 2)\
V(BINARY_MULTIPLY, 2)\
V(BINARY_DIVIDE, 2)\
V(BINARY_MODULO, 2)\
V(BINARY_LSHIFT, 2)\
V(BINARY_RSHIFT, 2)\
V(BINARY_RSHIFT_LOGICAL, 2)\
V(BINARY_LT, 2)\
V(BINARY_LTE, 2)\
V(BINARY_GT, 2)\
V(BINARY_GTE, 2)\
V(BINARY_INSTANCEOF, 2)\
V(BINARY_IN, 2)\
V(BINARY_EQ, 2)\
V(BINARY_STRICT_EQ, 2)\
V(BINARY_NE, 2)\
V(BINARY_STRICT_NE, 2)\
V(BINARY_BIT_AND, 2)\
V(BINARY_BIT_XOR, 2)\
V(BINARY_BIT_OR, 2)\
\
V(RETURN, 2)\
V(THROW, 2)\
V(POP_ENV, 1)\
V(WITH_SETUP, 2)\
V(RETURN_SUBROUTINE, 2)\
V(DEBUGGER, 1)\
\
V(LOAD_UNDEFINED, 2)\
V(LOAD_TRUE, 2)\
V(LOAD_FALSE, 2)\
V(LOAD_NULL, 2)\
V(LOAD_EMPTY, 2)\
\
V(LOAD_REGEXP, 2)\
V(LOAD_OBJECT, 3)\
\
V(LOAD_ELEMENT, 2)\
V(STORE_ELEMENT, 2)\
V(DELETE_ELEMENT, 2)\
V(INCREMENT_ELEMENT, 2)\
V(DECREMENT_ELEMENT, 2)\
V(POSTFIX_INCREMENT_ELEMENT, 2)\
V(POSTFIX_DECREMENT_ELEMENT, 2)\
\
V(RAISE_REFERENCE, 1)\
V(TO_NUMBER_AND_RAISE_REFERENCE, 2)\
\
V(TYPEOF, 2)\
\
V(STORE_OBJECT_DATA, 5)\
V(STORE_OBJECT_GET, 5)\
V(STORE_OBJECT_SET, 5)\
\
/* property lookup opcodes */\
V(LOAD_PROP, 7)\
V(LOAD_PROP_GENERIC, 7)\
V(LOAD_PROP_OWN, 7)\
V(LOAD_PROP_PROTO, 7)\
V(LOAD_PROP_CHAIN, 7)\
V(STORE_PROP, 6)\
V(STORE_PROP_GENERIC, 6)\
V(DELETE_PROP, 7)\
V(INCREMENT_PROP, 7)\
V(DECREMENT_PROP, 7)\
V(POSTFIX_INCREMENT_PROP, 7)\
V(POSTFIX_DECREMENT_PROP, 7)\
\
V(LOAD_CONST, 2)\
\
V(JUMP_BY, 2)\
V(JUMP_SUBROUTINE, 4)\
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
V(DELETE_NAME, 3)\
V(INCREMENT_NAME, 3)\
V(DECREMENT_NAME, 3)\
V(POSTFIX_INCREMENT_NAME, 3)\
V(POSTFIX_DECREMENT_NAME, 3)\
V(TYPEOF_NAME, 3)\
\
V(LOAD_GLOBAL, 5)\
V(STORE_GLOBAL, 5)\
V(DELETE_GLOBAL, 5)\
V(INCREMENT_GLOBAL, 5)\
V(DECREMENT_GLOBAL, 5)\
V(POSTFIX_INCREMENT_GLOBAL, 5)\
V(POSTFIX_DECREMENT_GLOBAL, 5)\
V(TYPEOF_GLOBAL, 5)\
\
V(LOAD_HEAP, 5)\
V(STORE_HEAP, 5)\
V(DELETE_HEAP, 5)\
V(INCREMENT_HEAP, 5)\
V(DECREMENT_HEAP, 5)\
V(POSTFIX_INCREMENT_HEAP, 5)\
V(POSTFIX_DECREMENT_HEAP, 5)\
V(TYPEOF_HEAP, 5)\
\
V(INCREMENT, 2)\
V(DECREMENT, 2)\
V(POSTFIX_INCREMENT, 2)\
V(POSTFIX_DECREMENT, 2)\
V(RAISE_IMMUTABLE, 2)\
V(PREPARE_DYNAMIC_CALL, 2)\
\
V(CALL, 3)\
V(CONSTRUCT, 3)\
V(EVAL, 3)\
\
V(LOAD_FUNCTION, 2)\
\
V(INIT_VECTOR_ARRAY_ELEMENT, 3)\
V(INIT_SPARSE_ARRAY_ELEMENT, 3)\
V(LOAD_ARRAY, 2)\
\
/* binding instantiations */\
V(BUILD_ENV, 2)\
V(INSTANTIATE_DECLARATION_BINDING, 2)\
V(INSTANTIATE_VARIABLE_BINDING, 2)\
V(INITIALIZE_HEAP_IMMUTABLE, 2)\
V(LOAD_ARGUMENTS, 2)

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

  static OP::Type ToHeap(uint8_t op) {
    assert(IsNameLookup(static_cast<OP::Type>(op)));
    return static_cast<OP::Type>(op + (OP::LOAD_HEAP - OP::LOAD_NAME));
  }

  static OP::Type BinaryOP(core::Token::Type token) {
    switch (token) {
      case core::Token::TK_ASSIGN_ADD:
      case core::Token::TK_ADD: return OP::BINARY_ADD;
      case core::Token::TK_ASSIGN_SUB:
      case core::Token::TK_SUB: return OP::BINARY_SUBTRACT;
      case core::Token::TK_ASSIGN_SHR:
      case core::Token::TK_SHR: return OP::BINARY_RSHIFT_LOGICAL;
      case core::Token::TK_ASSIGN_SAR:
      case core::Token::TK_SAR: return OP::BINARY_RSHIFT;
      case core::Token::TK_ASSIGN_SHL:
      case core::Token::TK_SHL: return OP::BINARY_LSHIFT;
      case core::Token::TK_ASSIGN_MUL:
      case core::Token::TK_MUL: return OP::BINARY_MULTIPLY;
      case core::Token::TK_ASSIGN_DIV:
      case core::Token::TK_DIV: return OP::BINARY_DIVIDE;
      case core::Token::TK_ASSIGN_MOD:
      case core::Token::TK_MOD: return OP::BINARY_MODULO;
      case core::Token::TK_LT: return OP::BINARY_LT;
      case core::Token::TK_GT: return OP::BINARY_GT;
      case core::Token::TK_LTE: return OP::BINARY_LTE;
      case core::Token::TK_GTE: return OP::BINARY_GTE;
      case core::Token::TK_INSTANCEOF: return OP::BINARY_INSTANCEOF;
      case core::Token::TK_IN: return OP::BINARY_IN;
      case core::Token::TK_EQ: return OP::BINARY_EQ;
      case core::Token::TK_NE: return OP::BINARY_NE;
      case core::Token::TK_EQ_STRICT: return OP::BINARY_STRICT_EQ;
      case core::Token::TK_NE_STRICT: return OP::BINARY_STRICT_NE;
      case core::Token::TK_ASSIGN_BIT_AND:
      case core::Token::TK_BIT_AND: return OP::BINARY_BIT_AND;
      case core::Token::TK_ASSIGN_BIT_XOR:
      case core::Token::TK_BIT_XOR: return OP::BINARY_BIT_XOR;
      case core::Token::TK_ASSIGN_BIT_OR:
      case core::Token::TK_BIT_OR: return OP::BINARY_BIT_OR;
      default: {
        UNREACHABLE();
      }
    }
    return OP::NOP;  // makes compiler happy
  }

  static OP::Type UnaryOP(core::Token::Type token) {
    switch (token) {
      case core::Token::TK_NOT: return OP::UNARY_NOT;
      case core::Token::TK_ADD: return OP::UNARY_POSITIVE;
      case core::Token::TK_SUB: return OP::UNARY_NEGATIVE;
      case core::Token::TK_BIT_NOT: return OP::UNARY_BIT_NOT;
      default: {
        UNREACHABLE();
      }
    }
    return OP::NOP;  // makes compiler happy
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
