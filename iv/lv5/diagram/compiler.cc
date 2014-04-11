// diagram::Compiler
//
// This compiler parses railgun::opcodes, inlines functions, and optimizes them
// with LLVM.
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/LLVMContext.h>

#include <iv/lv5/diagram/compiler.h>
namespace iv {
namespace lv5 {
namespace diagram {

typedef railgun::Instruction Instruction;
typedef Compiler::CompileStatus CompileStatus;

Compiler::~Compiler() {
  llvm::ExecutionEngine *EE =
      llvm::EngineBuilder(module_).setUseMCJIT(true).create();
  EE->finalizeObject();
  llvm::Function *F;
  // link entry points
  for (const auto& pair : entry_points_) {
    F = module_->getFunction(llvm::StringRef(pair.second));
    pair.first->set_executable(EE->getPointerToFunction(F));
  }
}

CompileStatus Compiler::CompileNOP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileENTER(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileMV(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBUILD_ENV(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileWITH_SETUP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileRETURN_SUBROUTINE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_CONST(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_ADD(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_SUBTRACT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_MULTIPLY(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_DIVIDE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_MODULO(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_LSHIFT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_RSHIFT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_RSHIFT_LOGICAL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_LT(const Instruction* instr,
                                railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_LTE(const Instruction* instr,
                                railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_GT(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_GTE(const Instruction* instr,
                                railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileCompare(const Instruction* instr, railgun::OP::Type fused) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_INSTANCEOF(const Instruction* instr,
                                       railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_IN(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_EQ(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_STRICT_EQ(const Instruction* instr,
                                      railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_NE(const Instruction* instr,
                               railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_STRICT_NE(const Instruction* instr,
                                      railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_BIT_AND(const Instruction* instr,
                                    railgun::OP::Type fused = railgun::OP::NOP) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_BIT_XOR(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileBINARY_BIT_OR(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileUNARY_POSITIVE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileUNARY_NEGATIVE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileUNARY_NOT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileUNARY_BIT_NOT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTHROW(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDEBUGGER(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTO_NUMBER(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTO_PRIMITIVE_AND_TO_STRING(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileCONCAT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileRAISE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTYPEOF(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_OBJECT_INDEXED(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_OBJECT_DATA(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_OBJECT_GET(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_OBJECT_SET(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDELETE_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINCREMENT_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDECREMENT_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_INCREMENT_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_DECREMENT_PROP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOP_ENV(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINIT_VECTOR_ARRAY_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINIT_SPARSE_ARRAY_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_ARRAY(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDUP_ARRAY(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_FUNCTION(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_REGEXP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_OBJECT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDELETE_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINCREMENT_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDECREMENT_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_INCREMENT_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_DECREMENT_ELEMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileRESULT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileRETURN(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_GLOBAL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_GLOBAL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_GLOBAL_DIRECT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_GLOBAL_DIRECT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDELETE_GLOBAL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTYPEOF_GLOBAL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDELETE_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINCREMENT_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDECREMENT_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_INCREMENT_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_DECREMENT_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTYPEOF_HEAP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileCALL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileCONSTRUCT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileEVAL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINSTANTIATE_DECLARATION_BINDING(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINSTANTIATE_VARIABLE_BINDING(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINITIALIZE_HEAP_IMMUTABLE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINCREMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDECREMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_INCREMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_DECREMENT(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePREPARE_DYNAMIC_CALL(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileIF_FALSE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileIF_TRUE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileJUMP_SUBROUTINE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileFORIN_SETUP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileFORIN_ENUMERATE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileFORIN_LEAVE(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTRY_CATCH_SETUP(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileSTORE_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDELETE_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileINCREMENT_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileDECREMENT_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_INCREMENT_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompilePOSTFIX_DECREMENT_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileTYPEOF_NAME(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileJUMP_BY(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}
CompileStatus Compiler::CompileLOAD_ARGUMENTS(const Instruction* instr) {
  return CompileStatus_NotCompiled;
}

CompileStatus Compiler::Main() {
  namespace r = railgun;
  namespace d = diagram;

  const Instruction* instr = code_->begin();
  for (const Instruction* last = code_->end(); instr != last;) {
    CompileStatus status = CompileStatus_Compiled;
    const uint32_t opcode = instr->GetOP();
    switch (opcode) {
      case r::OP::NOP:
        status = CompileNOP(instr);
        break;
      case r::OP::ENTER:
        status = CompileENTER(instr);
        break;
      case r::OP::MV:
        status = CompileMV(instr);
        break;
      case r::OP::UNARY_POSITIVE:
        status = CompileUNARY_POSITIVE(instr);
        break;
      case r::OP::UNARY_NEGATIVE:
        status = CompileUNARY_NEGATIVE(instr);
        break;
      case r::OP::UNARY_NOT:
        status = CompileUNARY_NOT(instr);
        break;
      case r::OP::UNARY_BIT_NOT:
        status = CompileUNARY_BIT_NOT(instr);
        break;
      case r::OP::BINARY_ADD:
        status = CompileBINARY_ADD(instr);
        break;
      case r::OP::BINARY_SUBTRACT:
        status = CompileBINARY_SUBTRACT(instr);
        break;
      case r::OP::BINARY_MULTIPLY:
        status = CompileBINARY_MULTIPLY(instr);
        break;
      case r::OP::BINARY_DIVIDE:
        status = CompileBINARY_DIVIDE(instr);
        break;
      case r::OP::BINARY_MODULO:
        status = CompileBINARY_MODULO(instr);
        break;
      case r::OP::BINARY_LSHIFT:
        status = CompileBINARY_LSHIFT(instr);
        break;
      case r::OP::BINARY_RSHIFT:
        status = CompileBINARY_RSHIFT(instr);
        break;
      case r::OP::BINARY_RSHIFT_LOGICAL:
        status = CompileBINARY_RSHIFT_LOGICAL(instr);
        break;
      case r::OP::BINARY_LT:
        status = CompileBINARY_LT(instr);
        break;
      case r::OP::BINARY_LTE:
        status = CompileBINARY_LTE(instr);
        break;
      case r::OP::BINARY_GT:
        status = CompileBINARY_GT(instr);
        break;
      case r::OP::BINARY_GTE:
        status = CompileBINARY_GTE(instr);
        break;
      case r::OP::BINARY_INSTANCEOF:
        status = CompileBINARY_INSTANCEOF(instr);
        break;
      case r::OP::BINARY_IN:
        status = CompileBINARY_IN(instr);
        break;
      case r::OP::BINARY_EQ:
        status = CompileBINARY_EQ(instr);
        break;
      case r::OP::BINARY_STRICT_EQ:
        status = CompileBINARY_STRICT_EQ(instr);
        break;
      case r::OP::BINARY_NE:
        status = CompileBINARY_NE(instr);
        break;
      case r::OP::BINARY_STRICT_NE:
        status = CompileBINARY_STRICT_NE(instr);
        break;
      case r::OP::BINARY_BIT_AND:
        status = CompileBINARY_BIT_AND(instr);
        break;
      case r::OP::BINARY_BIT_XOR:
        status = CompileBINARY_BIT_XOR(instr);
        break;
      case r::OP::BINARY_BIT_OR:
        status = CompileBINARY_BIT_OR(instr);
        break;
      case r::OP::RETURN:
        status = CompileRETURN(instr);
        break;
      case r::OP::THROW:
        status = CompileTHROW(instr);
        break;
      case r::OP::POP_ENV:
        status = CompilePOP_ENV(instr);
        break;
      case r::OP::WITH_SETUP:
        status = CompileWITH_SETUP(instr);
        break;
      case r::OP::RETURN_SUBROUTINE:
        status = CompileRETURN_SUBROUTINE(instr);
        break;
      case r::OP::DEBUGGER:
        status = CompileDEBUGGER(instr);
        break;
      case r::OP::LOAD_REGEXP:
        status = CompileLOAD_REGEXP(instr);
        break;
      case r::OP::LOAD_OBJECT:
        status = CompileLOAD_OBJECT(instr);
        break;
      case r::OP::LOAD_ELEMENT:
        status = CompileLOAD_ELEMENT(instr);
        break;
      case r::OP::STORE_ELEMENT:
        status = CompileSTORE_ELEMENT(instr);
        break;
      case r::OP::DELETE_ELEMENT:
        status = CompileDELETE_ELEMENT(instr);
        break;
      case r::OP::INCREMENT_ELEMENT:
        status = CompileINCREMENT_ELEMENT(instr);
        break;
      case r::OP::DECREMENT_ELEMENT:
        status = CompileDECREMENT_ELEMENT(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_ELEMENT:
        status = CompilePOSTFIX_INCREMENT_ELEMENT(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_ELEMENT:
        status = CompilePOSTFIX_DECREMENT_ELEMENT(instr);
        break;
      case r::OP::TO_NUMBER:
        status = CompileTO_NUMBER(instr);
        break;
      case r::OP::TO_PRIMITIVE_AND_TO_STRING:
        status = CompileTO_PRIMITIVE_AND_TO_STRING(instr);
        break;
      case r::OP::CONCAT:
        status = CompileCONCAT(instr);
        break;
      case r::OP::RAISE:
        status = CompileRAISE(instr);
        break;
      case r::OP::TYPEOF:
        status = CompileTYPEOF(instr);
        break;
      case r::OP::STORE_OBJECT_INDEXED:
        status = CompileSTORE_OBJECT_INDEXED(instr);
        break;
      case r::OP::STORE_OBJECT_DATA:
        status = CompileSTORE_OBJECT_DATA(instr);
        break;
      case r::OP::STORE_OBJECT_GET:
        status = CompileSTORE_OBJECT_GET(instr);
        break;
      case r::OP::STORE_OBJECT_SET:
        status = CompileSTORE_OBJECT_SET(instr);
        break;
      case r::OP::LOAD_PROP:
        status = CompileLOAD_PROP(instr);
        break;
      case r::OP::LOAD_PROP_GENERIC:
      case r::OP::LOAD_PROP_OWN:
      case r::OP::LOAD_PROP_PROTO:
      case r::OP::LOAD_PROP_CHAIN:
        UNREACHABLE();
        break;
      case r::OP::STORE_PROP:
        status = CompileSTORE_PROP(instr);
        break;
      case r::OP::STORE_PROP_GENERIC:
        UNREACHABLE();
        break;
      case r::OP::DELETE_PROP:
        status = CompileDELETE_PROP(instr);
        break;
      case r::OP::INCREMENT_PROP:
        status = CompileINCREMENT_PROP(instr);
        break;
      case r::OP::DECREMENT_PROP:
        status = CompileDECREMENT_PROP(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_PROP:
        status = CompilePOSTFIX_INCREMENT_PROP(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_PROP:
        status = CompilePOSTFIX_DECREMENT_PROP(instr);
        break;
      case r::OP::LOAD_CONST:
        status = CompileLOAD_CONST(instr);
        break;
      case r::OP::JUMP_BY:
        status = CompileJUMP_BY(instr);
        break;
      case r::OP::JUMP_SUBROUTINE:
        status = CompileJUMP_SUBROUTINE(instr);
        break;
      case r::OP::IF_FALSE:
        status = CompileIF_FALSE(instr);
        break;
      case r::OP::IF_TRUE:
        status = CompileIF_TRUE(instr);
        break;
      case r::OP::IF_FALSE_BINARY_LT:
        status = CompileBINARY_LT(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_LT:
        status = CompileBINARY_LT(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_LTE:
        status = CompileBINARY_LTE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_LTE:
        status = CompileBINARY_LTE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_GT:
        status = CompileBINARY_GT(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_GT:
        status = CompileBINARY_GT(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_GTE:
        status = CompileBINARY_GTE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_GTE:
        status = CompileBINARY_GTE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_INSTANCEOF:
        status = CompileBINARY_INSTANCEOF(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_INSTANCEOF:
        status = CompileBINARY_INSTANCEOF(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_IN:
        status = CompileBINARY_IN(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_IN:
        status = CompileBINARY_IN(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_EQ:
        status = CompileBINARY_EQ(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_EQ:
        status = CompileBINARY_EQ(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_NE:
        status = CompileBINARY_NE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_NE:
        status = CompileBINARY_NE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_STRICT_EQ:
        status = CompileBINARY_STRICT_EQ(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_STRICT_EQ:
        status = CompileBINARY_STRICT_EQ(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_STRICT_NE:
        status = CompileBINARY_STRICT_NE(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_STRICT_NE:
        status = CompileBINARY_STRICT_NE(instr, OP::IF_TRUE);
        break;
      case r::OP::IF_FALSE_BINARY_BIT_AND:
        status = CompileBINARY_BIT_AND(instr, OP::IF_FALSE);
        break;
      case r::OP::IF_TRUE_BINARY_BIT_AND:
        status = CompileBINARY_BIT_AND(instr, OP::IF_TRUE);
        break;
      case r::OP::FORIN_SETUP:
        status = CompileFORIN_SETUP(instr);
        break;
      case r::OP::FORIN_ENUMERATE:
        status = CompileFORIN_ENUMERATE(instr);
        break;
      case r::OP::FORIN_LEAVE:
        status = CompileFORIN_LEAVE(instr);
        break;
      case r::OP::TRY_CATCH_SETUP:
        status = CompileTRY_CATCH_SETUP(instr);
        break;
      case r::OP::LOAD_NAME:
        status = CompileLOAD_NAME(instr);
        break;
      case r::OP::STORE_NAME:
        status = CompileSTORE_NAME(instr);
        break;
      case r::OP::DELETE_NAME:
        status = CompileDELETE_NAME(instr);
        break;
      case r::OP::INCREMENT_NAME:
        status = CompileINCREMENT_NAME(instr);
        break;
      case r::OP::DECREMENT_NAME:
        status = CompileDECREMENT_NAME(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_NAME:
        status = CompilePOSTFIX_INCREMENT_NAME(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_NAME:
        status = CompilePOSTFIX_DECREMENT_NAME(instr);
        break;
      case r::OP::TYPEOF_NAME:
        status = CompileTYPEOF_NAME(instr);
        break;
      case r::OP::LOAD_GLOBAL:
        status = CompileLOAD_GLOBAL(instr);
        break;
      case r::OP::LOAD_GLOBAL_DIRECT:
        status = CompileLOAD_GLOBAL_DIRECT(instr);
        break;
      case r::OP::STORE_GLOBAL:
        status = CompileSTORE_GLOBAL(instr);
        break;
      case r::OP::STORE_GLOBAL_DIRECT:
        status = CompileSTORE_GLOBAL_DIRECT(instr);
        break;
      case r::OP::DELETE_GLOBAL:
        status = CompileDELETE_GLOBAL(instr);
        break;
      case r::OP::TYPEOF_GLOBAL:
        status = CompileTYPEOF_GLOBAL(instr);
        break;
      case r::OP::LOAD_HEAP:
        status = CompileLOAD_HEAP(instr);
        break;
      case r::OP::STORE_HEAP:
        status = CompileSTORE_HEAP(instr);
        break;
      case r::OP::DELETE_HEAP:
        status = CompileDELETE_HEAP(instr);
        break;
      case r::OP::INCREMENT_HEAP:
        status = CompileINCREMENT_HEAP(instr);
        break;
      case r::OP::DECREMENT_HEAP:
        status = CompileDECREMENT_HEAP(instr);
        break;
      case r::OP::POSTFIX_INCREMENT_HEAP:
        status = CompilePOSTFIX_INCREMENT_HEAP(instr);
        break;
      case r::OP::POSTFIX_DECREMENT_HEAP:
        status = CompilePOSTFIX_DECREMENT_HEAP(instr);
        break;
      case r::OP::TYPEOF_HEAP:
        status = CompileTYPEOF_HEAP(instr);
        break;
      case r::OP::INCREMENT:
        status = CompileINCREMENT(instr);
        break;
      case r::OP::DECREMENT:
        status = CompileDECREMENT(instr);
        break;
      case r::OP::POSTFIX_INCREMENT:
        status = CompilePOSTFIX_INCREMENT(instr);
        break;
      case r::OP::POSTFIX_DECREMENT:
        status = CompilePOSTFIX_DECREMENT(instr);
        break;
      case r::OP::PREPARE_DYNAMIC_CALL:
        status = CompilePREPARE_DYNAMIC_CALL(instr);
        break;
      case r::OP::CALL:
        status = CompileCALL(instr);
        break;
      case r::OP::CONSTRUCT:
        status = CompileCONSTRUCT(instr);
        break;
      case r::OP::EVAL:
        status = CompileEVAL(instr);
        break;
      case r::OP::RESULT:
        status = CompileRESULT(instr);
        break;
      case r::OP::LOAD_FUNCTION:
        status = CompileLOAD_FUNCTION(instr);
        break;
      case r::OP::INIT_VECTOR_ARRAY_ELEMENT:
        status = CompileINIT_VECTOR_ARRAY_ELEMENT(instr);
        break;
      case r::OP::INIT_SPARSE_ARRAY_ELEMENT:
        status = CompileINIT_SPARSE_ARRAY_ELEMENT(instr);
        break;
      case r::OP::LOAD_ARRAY:
        status = CompileLOAD_ARRAY(instr);
        break;
      case r::OP::DUP_ARRAY:
        status = CompileDUP_ARRAY(instr);
        break;
      case r::OP::BUILD_ENV:
        status = CompileBUILD_ENV(instr);
        break;
      case r::OP::INSTANTIATE_DECLARATION_BINDING:
        status = CompileINSTANTIATE_DECLARATION_BINDING(instr);
        break;
      case r::OP::INSTANTIATE_VARIABLE_BINDING:
        status = CompileINSTANTIATE_VARIABLE_BINDING(instr);
        break;
      case r::OP::INITIALIZE_HEAP_IMMUTABLE:
        status = CompileINITIALIZE_HEAP_IMMUTABLE(instr);
        break;
      case r::OP::LOAD_ARGUMENTS:
        status = CompileLOAD_ARGUMENTS(instr);
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

} } } // namespace iv::lv5::diagram
