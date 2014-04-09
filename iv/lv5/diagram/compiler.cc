// diagram::Compiler
//
// This compiler parses railgun::opcodes, inlines functions, and optimizes them
// with LLVM.

#include "iv/lv5/diagram/compiler.h"
namespace iv {
namespace lv5 {
namespace diagram {

typedef railgun::Instruction Instruction;

void CompileNOP(const Instruction* instr);
void CompileENTER(const Instruction* instr);
void CompileMV(const Instruction* instr);
void CompileBUILD_ENV(const Instruction* instr);
void CompileWITH_SETUP(const Instruction* instr);
void CompileRETURN_SUBROUTINE(const Instruction* instr);
void CompileLOAD_CONST(const Instruction* instr);
void CompileBINARY_ADD(const Instruction* instr);
void CompileBINARY_SUBTRACT(const Instruction* instr);
void CompileBINARY_MULTIPLY(const Instruction* instr);
void CompileBINARY_DIVIDE(const Instruction* instr);
void CompileBINARY_MODULO(const Instruction* instr);
void CompileBINARY_LSHIFT(const Instruction* instr);
void CompileBINARY_RSHIFT(const Instruction* instr);
void CompileBINARY_RSHIFT_LOGICAL(const Instruction* instr);
void CompileBINARY_LT(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_LTE(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_GT(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_GTE(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileCompare(const Instruction* instr, railgun::OP::Type fused);
void CompileBINARY_INSTANCEOF(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_IN(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_EQ(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_STRICT_EQ(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_NE(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_STRICT_NE(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_BIT_AND(const Instruction* instr, railgun::OP::Type fused = railgun::OP::NOP);
void CompileBINARY_BIT_XOR(const Instruction* instr);
void CompileBINARY_BIT_OR(const Instruction* instr);
void CompileUNARY_POSITIVE(const Instruction* instr);
void CompileUNARY_NEGATIVE(const Instruction* instr);
void CompileUNARY_NOT(const Instruction* instr);
void CompileUNARY_BIT_NOT(const Instruction* instr);
void CompileTHROW(const Instruction* instr);
void CompileDEBUGGER(const Instruction* instr);
void CompileTO_NUMBER(const Instruction* instr);
void CompileTO_PRIMITIVE_AND_TO_STRING(const Instruction* instr);
void CompileCONCAT(const Instruction* instr);
void CompileRAISE(const Instruction* instr);
void CompileTYPEOF(const Instruction* instr);
void CompileSTORE_OBJECT_INDEXED(const Instruction* instr);
void CompileSTORE_OBJECT_DATA(const Instruction* instr);
void CompileSTORE_OBJECT_GET(const Instruction* instr);
void CompileSTORE_OBJECT_SET(const Instruction* instr);
void CompileLOAD_PROP(const Instruction* instr);
void CompileSTORE_PROP(const Instruction* instr);
void CompileDELETE_PROP(const Instruction* instr);
void CompileINCREMENT_PROP(const Instruction* instr);
void CompileDECREMENT_PROP(const Instruction* instr);
void CompilePOSTFIX_INCREMENT_PROP(const Instruction* instr);
void CompilePOSTFIX_DECREMENT_PROP(const Instruction* instr);
void CompilePOP_ENV(const Instruction* instr);
void CompileINIT_VECTOR_ARRAY_ELEMENT(const Instruction* instr);
void CompileINIT_SPARSE_ARRAY_ELEMENT(const Instruction* instr);
void CompileLOAD_ARRAY(const Instruction* instr);
void CompileDUP_ARRAY(const Instruction* instr);
void CompileLOAD_FUNCTION(const Instruction* instr);
void CompileLOAD_REGEXP(const Instruction* instr);
void CompileLOAD_OBJECT(const Instruction* instr);
void CompileLOAD_ELEMENT(const Instruction* instr);
void CompileSTORE_ELEMENT(const Instruction* instr);
void CompileDELETE_ELEMENT(const Instruction* instr);
void CompileINCREMENT_ELEMENT(const Instruction* instr);
void CompileDECREMENT_ELEMENT(const Instruction* instr);
void CompilePOSTFIX_INCREMENT_ELEMENT(const Instruction* instr);
void CompilePOSTFIX_DECREMENT_ELEMENT(const Instruction* instr);
void CompileRESULT(const Instruction* instr);
void CompileRETURN(const Instruction* instr);
void CompileLOAD_GLOBAL(const Instruction* instr);
void CompileSTORE_GLOBAL(const Instruction* instr);
void CompileLOAD_GLOBAL_DIRECT(const Instruction* instr);
void CompileSTORE_GLOBAL_DIRECT(const Instruction* instr);
void CompileDELETE_GLOBAL(const Instruction* instr);
void CompileTYPEOF_GLOBAL(const Instruction* instr);
void CompileLOAD_HEAP(const Instruction* instr);
void CompileSTORE_HEAP(const Instruction* instr);
void CompileDELETE_HEAP(const Instruction* instr);
void CompileINCREMENT_HEAP(const Instruction* instr);
void CompileDECREMENT_HEAP(const Instruction* instr);
void CompilePOSTFIX_INCREMENT_HEAP(const Instruction* instr);
void CompilePOSTFIX_DECREMENT_HEAP(const Instruction* instr);
void CompileTYPEOF_HEAP(const Instruction* instr);
void CompileCALL(const Instruction* instr);
void CompileCONSTRUCT(const Instruction* instr);
void CompileEVAL(const Instruction* instr);
void CompileINSTANTIATE_DECLARATION_BINDING(const Instruction* instr);
void CompileINSTANTIATE_VARIABLE_BINDING(const Instruction* instr);
void CompileINITIALIZE_HEAP_IMMUTABLE(const Instruction* instr);
void CompileINCREMENT(const Instruction* instr);
void CompileDECREMENT(const Instruction* instr);
void CompilePOSTFIX_INCREMENT(const Instruction* instr);
void CompilePOSTFIX_DECREMENT(const Instruction* instr);
void CompilePREPARE_DYNAMIC_CALL(const Instruction* instr);
void CompileIF_FALSE(const Instruction* instr);
void CompileIF_TRUE(const Instruction* instr);
void CompileJUMP_SUBROUTINE(const Instruction* instr);
void CompileFORIN_SETUP(const Instruction* instr);
void CompileFORIN_ENUMERATE(const Instruction* instr);
void CompileFORIN_LEAVE(const Instruction* instr);
void CompileTRY_CATCH_SETUP(const Instruction* instr);
void CompileLOAD_NAME(const Instruction* instr);
void CompileSTORE_NAME(const Instruction* instr);
void CompileDELETE_NAME(const Instruction* instr);
void CompileINCREMENT_NAME(const Instruction* instr);
void CompileDECREMENT_NAME(const Instruction* instr);
void CompilePOSTFIX_INCREMENT_NAME(const Instruction* instr);
void CompilePOSTFIX_DECREMENT_NAME(const Instruction* instr);
void CompileTYPEOF_NAME(const Instruction* instr);
void CompileJUMP_BY(const Instruction* instr);
void CompileLOAD_ARGUMENTS(const Instruction* instr);

void Compiler::Main() {
  namespace r = railgun;
  namespace d = diagram;

  const Instruction* instr = code_->begin();
  for (const Instruction* last = code_->end(); instr != last;) {
      const uint32_t opcode = instr->GetOP();
    switch (opcode) {
      case r::OP::NOP:
        d::CompileNOP(instr);
        break;
      case r::OP::ENTER:
        d::CompileENTER(instr);
        break;
      case r::OP::MV:
        d::CompileMV(instr);
        break;
      case r::OP::UNARY_POSITIVE:
        d::CompileUNARY_POSITIVE(instr);
        break;
      case r::OP::UNARY_NEGATIVE:
        d::CompileUNARY_NEGATIVE(instr);
        break;
      case r::OP::UNARY_NOT:
        d::CompileUNARY_NOT(instr);
        break;
      case r::OP::UNARY_BIT_NOT:
        d::CompileUNARY_BIT_NOT(instr);
        break;
      case r::OP::BINARY_ADD:
        d::CompileBINARY_ADD(instr);
        break;
      case r::OP::BINARY_SUBTRACT:
        d::CompileBINARY_SUBTRACT(instr);
        break;
      case r::OP::BINARY_MULTIPLY:
        d::CompileBINARY_MULTIPLY(instr);
        break;
      case r::OP::BINARY_DIVIDE:
        d::CompileBINARY_DIVIDE(instr);
        break;
      case r::OP::BINARY_MODULO:
        d::CompileBINARY_MODULO(instr);
        break;
      case r::OP::BINARY_LSHIFT:
        d::CompileBINARY_LSHIFT(instr);
        break;
      case r::OP::BINARY_RSHIFT:
        d::CompileBINARY_RSHIFT(instr);
        break;
      case r::OP::BINARY_RSHIFT_LOGICAL:
        d::CompileBINARY_RSHIFT_LOGICAL(instr);
        break;
      case r::OP::BINARY_LT:
        d::CompileBINARY_LT(instr);
        break;
      case r::OP::BINARY_LTE:
        d::CompileBINARY_LTE(instr);
        break;
      case r::OP::BINARY_GT:
        d::CompileBINARY_GT(instr);
        break;
      case r::OP::BINARY_GTE:
        d::CompileBINARY_GTE(instr);
        break;
      case r::OP::BINARY_INSTANCEOF:
        d::CompileBINARY_INSTANCEOF(instr);
        break;
      case r::OP::BINARY_IN:
        d::CompileBINARY_IN(instr);
        break;
      case r::OP::BINARY_EQ:
        d::CompileBINARY_EQ(instr);
        break;
      case r::OP::BINARY_STRICT_EQ:
        d::CompileBINARY_STRICT_EQ(instr);
        break;
      case r::OP::BINARY_NE:
        d::CompileBINARY_NE(instr);
        break;
      case r::OP::BINARY_STRICT_NE:
        d::CompileBINARY_STRICT_NE(instr);
        break;
      case r::OP::BINARY_BIT_AND:
        d::CompileBINARY_BIT_AND(instr);
        break;
      case r::OP::BINARY_BIT_XOR:
        d::CompileBINARY_BIT_XOR(instr);
        break;
      case r::OP::BINARY_BIT_OR:
        d::CompileBINARY_BIT_OR(instr);
        break;
      case r::OP::RETURN:
        d::CompileRETURN(instr);
        break;
      case r::OP::THROW:
        d::CompileTHROW(instr);
        break;
      case r::OP::POP_ENV:
        d::CompilePOP_ENV(instr);
        break;
      case r::OP::WITH_SETUP:
        d::CompileWITH_SETUP(instr);
        break;
      case r::OP::RETURN_SUBROUTINE:
        d::CompileRETURN_SUBROUTINE(instr);
        break;
      case r::OP::DEBUGGER:
        d::CompileDEBUGGER(instr);
        break;
      case r::OP::LOAD_REGEXP:
        d::CompileLOAD_REGEXP(instr);
        break;
      case r::OP::LOAD_OBJECT:
        d::CompileLOAD_OBJECT(instr);
        break;
      case r::OP::LOAD_ELEMENT:
        d::CompileLOAD_ELEMENT(instr);
        break;
      case r::OP::STORE_ELEMENT:
        d::CompileSTORE_ELEMENT(instr);
        break;
      case r::OP::DELETE_ELEMENT:
        d::CompileDELETE_ELEMENT(instr);
        break;
      case r::OP::INCREMENT_ELEMENT:
        d::CompileINCREMENT_ELEMENT(instr);
        break;
      case r::OP::DECREMENT_ELEMENT:
        d::CompileDECREMENT_ELEMENT(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_ELEMENT:
        d::CompilePOSTFIX_INCREMENT_ELEMENT(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_ELEMENT:
        d::CompilePOSTFIX_DECREMENT_ELEMENT(instr);
        break;
      case r::OP::TO_NUMBER:
        d::CompileTO_NUMBER(instr);
        break;
      case r::OP::TO_PRIMITIVE_AND_TO_STRING:
        d::CompileTO_PRIMITIVE_AND_TO_STRING(instr);
        break;
      case r::OP::CONCAT:
        d::CompileCONCAT(instr);
        break;
      case r::OP::RAISE:
        d::CompileRAISE(instr);
        break;
      case r::OP::TYPEOF:
        d::CompileTYPEOF(instr);
        break;
      case r::OP::STORE_OBJECT_INDEXED:
        d::CompileSTORE_OBJECT_INDEXED(instr);
        break;
      case r::OP::STORE_OBJECT_DATA:
        d::CompileSTORE_OBJECT_DATA(instr);
        break;
      case r::OP::STORE_OBJECT_GET:
        d::CompileSTORE_OBJECT_GET(instr);
        break;
      case r::OP::STORE_OBJECT_SET:
        d::CompileSTORE_OBJECT_SET(instr);
        break;
      case r::OP::LOAD_PROP:
        d::CompileLOAD_PROP(instr);
        break;
      case r::OP::LOAD_PROP_GENERIC:
      case r::OP::LOAD_PROP_OWN:
      case r::OP::LOAD_PROP_PROTO:
      case r::OP::LOAD_PROP_CHAIN:
        UNREACHABLE();
        break;
      case r::OP::STORE_PROP:
        d::CompileSTORE_PROP(instr);
        break;
      case r::OP::STORE_PROP_GENERIC:
        UNREACHABLE();
        break;
      case r::OP::DELETE_PROP:
        d::CompileDELETE_PROP(instr);
        break;
      case r::OP::INCREMENT_PROP:
        d::CompileINCREMENT_PROP(instr);
        break;
      case r::OP::DECREMENT_PROP:
        d::CompileDECREMENT_PROP(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_PROP:
        d::CompilePOSTFIX_INCREMENT_PROP(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_PROP:
        d::CompilePOSTFIX_DECREMENT_PROP(instr);
        break;
      case r::OP::LOAD_CONST:
        d::CompileLOAD_CONST(instr);
        break;
      case r::OP::JUMP_BY:
        d::CompileJUMP_BY(instr);
        break;
      case r::OP::JUMP_SUBROUTINE:
        d::CompileJUMP_SUBROUTINE(instr);
        break;
      case r::OP::IF_FALSE:
        d::CompileIF_FALSE(instr);
        break;
      case r::OP::IF_TRUE:
        d::CompileIF_TRUE(instr);
        break;
      case r::OP::IF_FALSE_BINARY_LT:
        d::CompileBINARY_LT(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_LT:
        d::CompileBINARY_LT(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_LTE:
        d::CompileBINARY_LTE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_LTE:
        d::CompileBINARY_LTE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_GT:
        d::CompileBINARY_GT(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_GT:
        d::CompileBINARY_GT(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_GTE:
        d::CompileBINARY_GTE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_GTE:
        d::CompileBINARY_GTE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_INSTANCEOF:
        d::CompileBINARY_INSTANCEOF(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_INSTANCEOF:
        d::CompileBINARY_INSTANCEOF(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_IN:
        d::CompileBINARY_IN(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_IN:
        d::CompileBINARY_IN(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_EQ:
        d::CompileBINARY_EQ(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_EQ:
        d::CompileBINARY_EQ(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_NE:
        d::CompileBINARY_NE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_NE:
        d::CompileBINARY_NE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_STRICT_EQ:
        d::CompileBINARY_STRICT_EQ(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_STRICT_EQ:
        d::CompileBINARY_STRICT_EQ(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_STRICT_NE:
        d::CompileBINARY_STRICT_NE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_STRICT_NE:
        d::CompileBINARY_STRICT_NE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_BIT_AND:
        d::CompileBINARY_BIT_AND(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_BIT_AND:
        d::CompileBINARY_BIT_AND(instr, OP::IF_TRUE);
        break;
      case r::OP::FORIN_SETUP:
        d::CompileFORIN_SETUP(instr);
        break;
      case r::OP::FORIN_ENUMERATE:
        d::CompileFORIN_ENUMERATE(instr);
        break;
      case r::OP::FORIN_LEAVE:
        d::CompileFORIN_LEAVE(instr);
        break;
      case r::OP::TRY_CATCH_SETUP:
        d::CompileTRY_CATCH_SETUP(instr);
        break;
      case r::OP::LOAD_NAME:
        d::CompileLOAD_NAME(instr);
        break;
      case r::OP::STORE_NAME:
        d::CompileSTORE_NAME(instr);
        break;
      case r::OP::DELETE_NAME:
        d::CompileDELETE_NAME(instr);
        break;
      case r::OP::INCREMENT_NAME:
        d::CompileINCREMENT_NAME(instr);
        break;
      case r::OP::DECREMENT_NAME:
        d::CompileDECREMENT_NAME(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_NAME:
        d::CompilePOSTFIX_INCREMENT_NAME(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_NAME:
        d::CompilePOSTFIX_DECREMENT_NAME(instr);
        break;
      case r::OP::TYPEOF_NAME:
        d::CompileTYPEOF_NAME(instr);
        break;
      case r::OP::LOAD_GLOBAL:
        d::CompileLOAD_GLOBAL(instr);
        break;
      case r::OP::LOAD_GLOBAL_DIRECT:
        d::CompileLOAD_GLOBAL_DIRECT(instr);
        break;
      case r::OP::STORE_GLOBAL:
        d::CompileSTORE_GLOBAL(instr);
        break;
      case r::OP::STORE_GLOBAL_DIRECT:
        d::CompileSTORE_GLOBAL_DIRECT(instr);
        break;
      case r::OP::DELETE_GLOBAL:
        d::CompileDELETE_GLOBAL(instr);
        break;
      case r::OP::TYPEOF_GLOBAL:
        d::CompileTYPEOF_GLOBAL(instr);
        break;
      case r::OP::LOAD_HEAP:
        d::CompileLOAD_HEAP(instr);
        break;
      case r::OP::STORE_HEAP:
        d::CompileSTORE_HEAP(instr);
        break;
      case r::OP::DELETE_HEAP:
        d::CompileDELETE_HEAP(instr);
        break;
      case r::OP::INCREMENT_HEAP:
        d::CompileINCREMENT_HEAP(instr);
        break;
      case r::OP::DECREMENT_HEAP:
        d::CompileDECREMENT_HEAP(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_HEAP:
        d::CompilePOSTFIX_INCREMENT_HEAP(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_HEAP:
        d::CompilePOSTFIX_DECREMENT_HEAP(instr);
        break;
      case r::OP::TYPEOF_HEAP:
        d::CompileTYPEOF_HEAP(instr);
        break;
      case r::OP::INCREMENT:
        d::CompileINCREMENT(instr);
        break;
      case r::OP::DECREMENT:
        d::CompileDECREMENT(instr);
        break;
      case r::OP::POSTFIX_INCREMENT:
        d::CompilePOSTFIX_INCREMENT(instr);
        break;
      case r::OP::POSTFIX_DECREMENT:
        d::CompilePOSTFIX_DECREMENT(instr);
        break;
      case r::OP::PREPARE_DYNAMIC_CALL:
        d::CompilePREPARE_DYNAMIC_CALL(instr);
        break;
      case r::OP::CALL:
        d::CompileCALL(instr);
        break;
      case r::OP::CONSTRUCT:
        d::CompileCONSTRUCT(instr);
        break;
      case r::OP::EVAL:
        d::CompileEVAL(instr);
        break;
      case r::OP::RESULT:
        d::CompileRESULT(instr);
        break;
      case r::OP::LOAD_FUNCTION:
        d::CompileLOAD_FUNCTION(instr);
        break;
      case r::OP::INIT_VECTOR_ARRAY_ELEMENT:
        d::CompileINIT_VECTOR_ARRAY_ELEMENT(instr);
        break;
      case r::OP::INIT_SPARSE_ARRAY_ELEMENT:
        d::CompileINIT_SPARSE_ARRAY_ELEMENT(instr);
        break;
      case r::OP::LOAD_ARRAY:
        d::CompileLOAD_ARRAY(instr);
        break;
      case r::OP::DUP_ARRAY:
        d::CompileDUP_ARRAY(instr);
        break;
      case r::OP::BUILD_ENV:
        d::CompileBUILD_ENV(instr);
        break;
      case r::OP::INSTANTIATE_DECLARATION_BINDING:
        d::CompileINSTANTIATE_DECLARATION_BINDING(instr);
        break;
      case r::OP::INSTANTIATE_VARIABLE_BINDING:
        d::CompileINSTANTIATE_VARIABLE_BINDING(instr);
        break;
      case r::OP::INITIALIZE_HEAP_IMMUTABLE:
        d::CompileINITIALIZE_HEAP_IMMUTABLE(instr);
        break;
      case r::OP::LOAD_ARGUMENTS:
        d::CompileLOAD_ARGUMENTS(instr);
        break;
    }
    const uint32_t length = r::kOPLength[opcode];
    std::advance(instr, length);
  }
}

} } }  // namespace iv::lv5::diagram