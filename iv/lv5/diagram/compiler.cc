// diagram::Compiler
//
// This compiler parses railgun::opcodes, inlines functions, and optimizes them
// with LLVM.

#include "iv/lv5/diagram/compiler.h"
namespace iv {
namespace lv5 {
namespace diagram {

typedef railgun::Instruction Instruction;
typedef Compiler::CompileStatus CompileStatus;

CompileStatus CompileNOP(const Instruction* instr);
CompileStatus CompileENTER(const Instruction* instr);
CompileStatus CompileMV(const Instruction* instr);
CompileStatus CompileBUILD_ENV(const Instruction* instr);
CompileStatus CompileWITH_SETUP(const Instruction* instr);
CompileStatus CompileRETURN_SUBROUTINE(const Instruction* instr);
CompileStatus CompileLOAD_CONST(const Instruction* instr);
CompileStatus CompileBINARY_ADD(const Instruction* instr);
CompileStatus CompileBINARY_SUBTRACT(const Instruction* instr);
CompileStatus CompileBINARY_MULTIPLY(const Instruction* instr);
CompileStatus CompileBINARY_DIVIDE(const Instruction* instr);
CompileStatus CompileBINARY_MODULO(const Instruction* instr);
CompileStatus CompileBINARY_LSHIFT(const Instruction* instr);
CompileStatus CompileBINARY_RSHIFT(const Instruction* instr);
CompileStatus CompileBINARY_RSHIFT_LOGICAL(const Instruction* instr);
CompileStatus CompileBINARY_LT(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_LTE(const Instruction* instr,
                                railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_GT(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_GTE(const Instruction* instr,
                                railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileCompare(const Instruction* instr, railgun::OP::Type fused);
CompileStatus CompileBINARY_INSTANCEOF(const Instruction* instr,
                                       railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_IN(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_EQ(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_STRICT_EQ(const Instruction* instr,
                                      railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_NE(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_STRICT_NE(const Instruction* instr,
                                      railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_BIT_AND(const Instruction* instr,
                                    railgun::OP::Type fused = railgun::OP::NOP);
CompileStatus CompileBINARY_BIT_XOR(const Instruction* instr);
CompileStatus CompileBINARY_BIT_OR(const Instruction* instr);
CompileStatus CompileUNARY_POSITIVE(const Instruction* instr);
CompileStatus CompileUNARY_NEGATIVE(const Instruction* instr);
CompileStatus CompileUNARY_NOT(const Instruction* instr);
CompileStatus CompileUNARY_BIT_NOT(const Instruction* instr);
CompileStatus CompileTHROW(const Instruction* instr);
CompileStatus CompileDEBUGGER(const Instruction* instr);
CompileStatus CompileTO_NUMBER(const Instruction* instr);
CompileStatus CompileTO_PRIMITIVE_AND_TO_STRING(const Instruction* instr);
CompileStatus CompileCONCAT(const Instruction* instr);
CompileStatus CompileRAISE(const Instruction* instr);
CompileStatus CompileTYPEOF(const Instruction* instr);
CompileStatus CompileSTORE_OBJECT_INDEXED(const Instruction* instr);
CompileStatus CompileSTORE_OBJECT_DATA(const Instruction* instr);
CompileStatus CompileSTORE_OBJECT_GET(const Instruction* instr);
CompileStatus CompileSTORE_OBJECT_SET(const Instruction* instr);
CompileStatus CompileLOAD_PROP(const Instruction* instr);
CompileStatus CompileSTORE_PROP(const Instruction* instr);
CompileStatus CompileDELETE_PROP(const Instruction* instr);
CompileStatus CompileINCREMENT_PROP(const Instruction* instr);
CompileStatus CompileDECREMENT_PROP(const Instruction* instr);
CompileStatus CompilePOSTFIX_INCREMENT_PROP(const Instruction* instr);
CompileStatus CompilePOSTFIX_DECREMENT_PROP(const Instruction* instr);
CompileStatus CompilePOP_ENV(const Instruction* instr);
CompileStatus CompileINIT_VECTOR_ARRAY_ELEMENT(const Instruction* instr);
CompileStatus CompileINIT_SPARSE_ARRAY_ELEMENT(const Instruction* instr);
CompileStatus CompileLOAD_ARRAY(const Instruction* instr);
CompileStatus CompileDUP_ARRAY(const Instruction* instr);
CompileStatus CompileLOAD_FUNCTION(const Instruction* instr);
CompileStatus CompileLOAD_REGEXP(const Instruction* instr);
CompileStatus CompileLOAD_OBJECT(const Instruction* instr);
CompileStatus CompileLOAD_ELEMENT(const Instruction* instr);
CompileStatus CompileSTORE_ELEMENT(const Instruction* instr);
CompileStatus CompileDELETE_ELEMENT(const Instruction* instr);
CompileStatus CompileINCREMENT_ELEMENT(const Instruction* instr);
CompileStatus CompileDECREMENT_ELEMENT(const Instruction* instr);
CompileStatus CompilePOSTFIX_INCREMENT_ELEMENT(const Instruction* instr);
CompileStatus CompilePOSTFIX_DECREMENT_ELEMENT(const Instruction* instr);
CompileStatus CompileRESULT(const Instruction* instr);
CompileStatus CompileRETURN(const Instruction* instr);
CompileStatus CompileLOAD_GLOBAL(const Instruction* instr);
CompileStatus CompileSTORE_GLOBAL(const Instruction* instr);
CompileStatus CompileLOAD_GLOBAL_DIRECT(const Instruction* instr);
CompileStatus CompileSTORE_GLOBAL_DIRECT(const Instruction* instr);
CompileStatus CompileDELETE_GLOBAL(const Instruction* instr);
CompileStatus CompileTYPEOF_GLOBAL(const Instruction* instr);
CompileStatus CompileLOAD_HEAP(const Instruction* instr);
CompileStatus CompileSTORE_HEAP(const Instruction* instr);
CompileStatus CompileDELETE_HEAP(const Instruction* instr);
CompileStatus CompileINCREMENT_HEAP(const Instruction* instr);
CompileStatus CompileDECREMENT_HEAP(const Instruction* instr);
CompileStatus CompilePOSTFIX_INCREMENT_HEAP(const Instruction* instr);
CompileStatus CompilePOSTFIX_DECREMENT_HEAP(const Instruction* instr);
CompileStatus CompileTYPEOF_HEAP(const Instruction* instr);
CompileStatus CompileCALL(const Instruction* instr);
CompileStatus CompileCONSTRUCT(const Instruction* instr);
CompileStatus CompileEVAL(const Instruction* instr);
CompileStatus CompileINSTANTIATE_DECLARATION_BINDING(const Instruction* instr);
CompileStatus CompileINSTANTIATE_VARIABLE_BINDING(const Instruction* instr);
CompileStatus CompileINITIALIZE_HEAP_IMMUTABLE(const Instruction* instr);
CompileStatus CompileINCREMENT(const Instruction* instr);
CompileStatus CompileDECREMENT(const Instruction* instr);
CompileStatus CompilePOSTFIX_INCREMENT(const Instruction* instr);
CompileStatus CompilePOSTFIX_DECREMENT(const Instruction* instr);
CompileStatus CompilePREPARE_DYNAMIC_CALL(const Instruction* instr);
CompileStatus CompileIF_FALSE(const Instruction* instr);
CompileStatus CompileIF_TRUE(const Instruction* instr);
CompileStatus CompileJUMP_SUBROUTINE(const Instruction* instr);
CompileStatus CompileFORIN_SETUP(const Instruction* instr);
CompileStatus CompileFORIN_ENUMERATE(const Instruction* instr);
CompileStatus CompileFORIN_LEAVE(const Instruction* instr);
CompileStatus CompileTRY_CATCH_SETUP(const Instruction* instr);
CompileStatus CompileLOAD_NAME(const Instruction* instr);
CompileStatus CompileSTORE_NAME(const Instruction* instr);
CompileStatus CompileDELETE_NAME(const Instruction* instr);
CompileStatus CompileINCREMENT_NAME(const Instruction* instr);
CompileStatus CompileDECREMENT_NAME(const Instruction* instr);
CompileStatus CompilePOSTFIX_INCREMENT_NAME(const Instruction* instr);
CompileStatus CompilePOSTFIX_DECREMENT_NAME(const Instruction* instr);
CompileStatus CompileTYPEOF_NAME(const Instruction* instr);
CompileStatus CompileJUMP_BY(const Instruction* instr);
CompileStatus CompileLOAD_ARGUMENTS(const Instruction* instr);

CompileStatus Compiler::Main() {
  namespace r = railgun;
  namespace d = diagram;

  const Instruction* instr = code_->begin();
  for (const Instruction* last = code_->end(); instr != last;) {
    CompileStatus status = CompileStatus_Compiled;
    const uint32_t opcode = instr->GetOP();
    switch (opcode) {
      case r::OP::NOP:
        status = d::CompileNOP(instr);
        break;
      case r::OP::ENTER:
        status = d::CompileENTER(instr);
        break;
      case r::OP::MV:
        status = d::CompileMV(instr);
        break;
      case r::OP::UNARY_POSITIVE:
        status = d::CompileUNARY_POSITIVE(instr);
        break;
      case r::OP::UNARY_NEGATIVE:
        status = d::CompileUNARY_NEGATIVE(instr);
        break;
      case r::OP::UNARY_NOT:
        status = d::CompileUNARY_NOT(instr);
        break;
      case r::OP::UNARY_BIT_NOT:
        status = d::CompileUNARY_BIT_NOT(instr);
        break;
      case r::OP::BINARY_ADD:
        status = d::CompileBINARY_ADD(instr);
        break;
      case r::OP::BINARY_SUBTRACT:
        status = d::CompileBINARY_SUBTRACT(instr);
        break;
      case r::OP::BINARY_MULTIPLY:
        status = d::CompileBINARY_MULTIPLY(instr);
        break;
      case r::OP::BINARY_DIVIDE:
        status = d::CompileBINARY_DIVIDE(instr);
        break;
      case r::OP::BINARY_MODULO:
        status = d::CompileBINARY_MODULO(instr);
        break;
      case r::OP::BINARY_LSHIFT:
        status = d::CompileBINARY_LSHIFT(instr);
        break;
      case r::OP::BINARY_RSHIFT:
        status = d::CompileBINARY_RSHIFT(instr);
        break;
      case r::OP::BINARY_RSHIFT_LOGICAL:
        status = d::CompileBINARY_RSHIFT_LOGICAL(instr);
        break;
      case r::OP::BINARY_LT:
        status = d::CompileBINARY_LT(instr);
        break;
      case r::OP::BINARY_LTE:
        status = d::CompileBINARY_LTE(instr);
        break;
      case r::OP::BINARY_GT:
        status = d::CompileBINARY_GT(instr);
        break;
      case r::OP::BINARY_GTE:
        status = d::CompileBINARY_GTE(instr);
        break;
      case r::OP::BINARY_INSTANCEOF:
        status = d::CompileBINARY_INSTANCEOF(instr);
        break;
      case r::OP::BINARY_IN:
        status = d::CompileBINARY_IN(instr);
        break;
      case r::OP::BINARY_EQ:
        status = d::CompileBINARY_EQ(instr);
        break;
      case r::OP::BINARY_STRICT_EQ:
        status = d::CompileBINARY_STRICT_EQ(instr);
        break;
      case r::OP::BINARY_NE:
        status = d::CompileBINARY_NE(instr);
        break;
      case r::OP::BINARY_STRICT_NE:
        status = d::CompileBINARY_STRICT_NE(instr);
        break;
      case r::OP::BINARY_BIT_AND:
        status = d::CompileBINARY_BIT_AND(instr);
        break;
      case r::OP::BINARY_BIT_XOR:
        status = d::CompileBINARY_BIT_XOR(instr);
        break;
      case r::OP::BINARY_BIT_OR:
        status = d::CompileBINARY_BIT_OR(instr);
        break;
      case r::OP::RETURN:
        status = d::CompileRETURN(instr);
        break;
      case r::OP::THROW:
        status = d::CompileTHROW(instr);
        break;
      case r::OP::POP_ENV:
        status = d::CompilePOP_ENV(instr);
        break;
      case r::OP::WITH_SETUP:
        status = d::CompileWITH_SETUP(instr);
        break;
      case r::OP::RETURN_SUBROUTINE:
        status = d::CompileRETURN_SUBROUTINE(instr);
        break;
      case r::OP::DEBUGGER:
        status = d::CompileDEBUGGER(instr);
        break;
      case r::OP::LOAD_REGEXP:
        status = d::CompileLOAD_REGEXP(instr);
        break;
      case r::OP::LOAD_OBJECT:
        status = d::CompileLOAD_OBJECT(instr);
        break;
      case r::OP::LOAD_ELEMENT:
        status = d::CompileLOAD_ELEMENT(instr);
        break;
      case r::OP::STORE_ELEMENT:
        status = d::CompileSTORE_ELEMENT(instr);
        break;
      case r::OP::DELETE_ELEMENT:
        status = d::CompileDELETE_ELEMENT(instr);
        break;
      case r::OP::INCREMENT_ELEMENT:
        status = d::CompileINCREMENT_ELEMENT(instr);
        break;
      case r::OP::DECREMENT_ELEMENT:
        status = d::CompileDECREMENT_ELEMENT(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_ELEMENT:
        status = d::CompilePOSTFIX_INCREMENT_ELEMENT(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_ELEMENT:
        status = d::CompilePOSTFIX_DECREMENT_ELEMENT(instr);
        break;
      case r::OP::TO_NUMBER:
        status = d::CompileTO_NUMBER(instr);
        break;
      case r::OP::TO_PRIMITIVE_AND_TO_STRING:
        status = d::CompileTO_PRIMITIVE_AND_TO_STRING(instr);
        break;
      case r::OP::CONCAT:
        status = d::CompileCONCAT(instr);
        break;
      case r::OP::RAISE:
        status = d::CompileRAISE(instr);
        break;
      case r::OP::TYPEOF:
        status = d::CompileTYPEOF(instr);
        break;
      case r::OP::STORE_OBJECT_INDEXED:
        status = d::CompileSTORE_OBJECT_INDEXED(instr);
        break;
      case r::OP::STORE_OBJECT_DATA:
        status = d::CompileSTORE_OBJECT_DATA(instr);
        break;
      case r::OP::STORE_OBJECT_GET:
        status = d::CompileSTORE_OBJECT_GET(instr);
        break;
      case r::OP::STORE_OBJECT_SET:
        status = d::CompileSTORE_OBJECT_SET(instr);
        break;
      case r::OP::LOAD_PROP:
        status = d::CompileLOAD_PROP(instr);
        break;
      case r::OP::LOAD_PROP_GENERIC:
      case r::OP::LOAD_PROP_OWN:
      case r::OP::LOAD_PROP_PROTO:
      case r::OP::LOAD_PROP_CHAIN:
        UNREACHABLE();
        break;
      case r::OP::STORE_PROP:
        status = d::CompileSTORE_PROP(instr);
        break;
      case r::OP::STORE_PROP_GENERIC:
        UNREACHABLE();
        break;
      case r::OP::DELETE_PROP:
        status = d::CompileDELETE_PROP(instr);
        break;
      case r::OP::INCREMENT_PROP:
        status = d::CompileINCREMENT_PROP(instr);
        break;
      case r::OP::DECREMENT_PROP:
        status = d::CompileDECREMENT_PROP(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_PROP:
        status = d::CompilePOSTFIX_INCREMENT_PROP(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_PROP:
        status = d::CompilePOSTFIX_DECREMENT_PROP(instr);
        break;
      case r::OP::LOAD_CONST:
        status = d::CompileLOAD_CONST(instr);
        break;
      case r::OP::JUMP_BY:
        status = d::CompileJUMP_BY(instr);
        break;
      case r::OP::JUMP_SUBROUTINE:
        status = d::CompileJUMP_SUBROUTINE(instr);
        break;
      case r::OP::IF_FALSE:
        status = d::CompileIF_FALSE(instr);
        break;
      case r::OP::IF_TRUE:
        status = d::CompileIF_TRUE(instr);
        break;
      case r::OP::IF_FALSE_BINARY_LT:
        status = d::CompileBINARY_LT(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_LT:
        status = d::CompileBINARY_LT(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_LTE:
        status = d::CompileBINARY_LTE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_LTE:
        status = d::CompileBINARY_LTE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_GT:
        status = d::CompileBINARY_GT(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_GT:
        status = d::CompileBINARY_GT(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_GTE:
        status = d::CompileBINARY_GTE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_GTE:
        status = d::CompileBINARY_GTE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_INSTANCEOF:
        status = d::CompileBINARY_INSTANCEOF(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_INSTANCEOF:
        status = d::CompileBINARY_INSTANCEOF(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_IN:
        status = d::CompileBINARY_IN(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_IN:
        status = d::CompileBINARY_IN(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_EQ:
        status = d::CompileBINARY_EQ(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_EQ:
        status = d::CompileBINARY_EQ(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_NE:
        status = d::CompileBINARY_NE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_NE:
        status = d::CompileBINARY_NE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_STRICT_EQ:
        status = d::CompileBINARY_STRICT_EQ(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_STRICT_EQ:
        status = d::CompileBINARY_STRICT_EQ(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_STRICT_NE:
        status = d::CompileBINARY_STRICT_NE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_STRICT_NE:
        status = d::CompileBINARY_STRICT_NE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_BIT_AND:
        status = d::CompileBINARY_BIT_AND(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_BIT_AND:
        status = d::CompileBINARY_BIT_AND(instr, OP::IF_TRUE);
        break;
      case r::OP::FORIN_SETUP:
        status = d::CompileFORIN_SETUP(instr);
        break;
      case r::OP::FORIN_ENUMERATE:
        status = d::CompileFORIN_ENUMERATE(instr);
        break;
      case r::OP::FORIN_LEAVE:
        status = d::CompileFORIN_LEAVE(instr);
        break;
      case r::OP::TRY_CATCH_SETUP:
        status = d::CompileTRY_CATCH_SETUP(instr);
        break;
      case r::OP::LOAD_NAME:
        status = d::CompileLOAD_NAME(instr);
        break;
      case r::OP::STORE_NAME:
        status = d::CompileSTORE_NAME(instr);
        break;
      case r::OP::DELETE_NAME:
        status = d::CompileDELETE_NAME(instr);
        break;
      case r::OP::INCREMENT_NAME:
        status = d::CompileINCREMENT_NAME(instr);
        break;
      case r::OP::DECREMENT_NAME:
        status = d::CompileDECREMENT_NAME(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_NAME:
        status = d::CompilePOSTFIX_INCREMENT_NAME(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_NAME:
        status = d::CompilePOSTFIX_DECREMENT_NAME(instr);
        break;
      case r::OP::TYPEOF_NAME:
        status = d::CompileTYPEOF_NAME(instr);
        break;
      case r::OP::LOAD_GLOBAL:
        status = d::CompileLOAD_GLOBAL(instr);
        break;
      case r::OP::LOAD_GLOBAL_DIRECT:
        status = d::CompileLOAD_GLOBAL_DIRECT(instr);
        break;
      case r::OP::STORE_GLOBAL:
        status = d::CompileSTORE_GLOBAL(instr);
        break;
      case r::OP::STORE_GLOBAL_DIRECT:
        status = d::CompileSTORE_GLOBAL_DIRECT(instr);
        break;
      case r::OP::DELETE_GLOBAL:
        status = d::CompileDELETE_GLOBAL(instr);
        break;
      case r::OP::TYPEOF_GLOBAL:
        status = d::CompileTYPEOF_GLOBAL(instr);
        break;
      case r::OP::LOAD_HEAP:
        status = d::CompileLOAD_HEAP(instr);
        break;
      case r::OP::STORE_HEAP:
        status = d::CompileSTORE_HEAP(instr);
        break;
      case r::OP::DELETE_HEAP:
        status = d::CompileDELETE_HEAP(instr);
        break;
      case r::OP::INCREMENT_HEAP:
        status = d::CompileINCREMENT_HEAP(instr);
        break;
      case r::OP::DECREMENT_HEAP:
        status = d::CompileDECREMENT_HEAP(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_HEAP:
        status = d::CompilePOSTFIX_INCREMENT_HEAP(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_HEAP:
        status = d::CompilePOSTFIX_DECREMENT_HEAP(instr);
        break;
      case r::OP::TYPEOF_HEAP:
        status = d::CompileTYPEOF_HEAP(instr);
        break;
      case r::OP::INCREMENT:
        status = d::CompileINCREMENT(instr);
        break;
      case r::OP::DECREMENT:
        status = d::CompileDECREMENT(instr);
        break;
      case r::OP::POSTFIX_INCREMENT:
        status = d::CompilePOSTFIX_INCREMENT(instr);
        break;
      case r::OP::POSTFIX_DECREMENT:
        status = d::CompilePOSTFIX_DECREMENT(instr);
        break;
      case r::OP::PREPARE_DYNAMIC_CALL:
        status = d::CompilePREPARE_DYNAMIC_CALL(instr);
        break;
      case r::OP::CALL:
        status = d::CompileCALL(instr);
        break;
      case r::OP::CONSTRUCT:
        status = d::CompileCONSTRUCT(instr);
        break;
      case r::OP::EVAL:
        status = d::CompileEVAL(instr);
        break;
      case r::OP::RESULT:
        status = d::CompileRESULT(instr);
        break;
      case r::OP::LOAD_FUNCTION:
        status = d::CompileLOAD_FUNCTION(instr);
        break;
      case r::OP::INIT_VECTOR_ARRAY_ELEMENT:
        status = d::CompileINIT_VECTOR_ARRAY_ELEMENT(instr);
        break;
      case r::OP::INIT_SPARSE_ARRAY_ELEMENT:
        status = d::CompileINIT_SPARSE_ARRAY_ELEMENT(instr);
        break;
      case r::OP::LOAD_ARRAY:
        status = d::CompileLOAD_ARRAY(instr);
        break;
      case r::OP::DUP_ARRAY:
        status = d::CompileDUP_ARRAY(instr);
        break;
      case r::OP::BUILD_ENV:
        status = d::CompileBUILD_ENV(instr);
        break;
      case r::OP::INSTANTIATE_DECLARATION_BINDING:
        status = d::CompileINSTANTIATE_DECLARATION_BINDING(instr);
        break;
      case r::OP::INSTANTIATE_VARIABLE_BINDING:
        status = d::CompileINSTANTIATE_VARIABLE_BINDING(instr);
        break;
      case r::OP::INITIALIZE_HEAP_IMMUTABLE:
        status = d::CompileINITIALIZE_HEAP_IMMUTABLE(instr);
        break;
      case r::OP::LOAD_ARGUMENTS:
        status = d::CompileLOAD_ARGUMENTS(instr);
        break;
    }
    if (status != CompileStatus_Compiled) {
      return status;
    }
    const uint32_t length = r::kOPLength[opcode];
    std::advance(instr, length);
  }
  return CompileStatus_Compiled;
}

} } } // namespace iv::lv5::diagram::Compiler