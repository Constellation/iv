// diagram::Compiler
//
// This compiler parses railgun::opcodes, inlines functions, and optimizes them
// with LLVM.

#include "iv/lv5/diagram/compiler.h"
namespace iv {
namespace lv5 {
namespace diagram {

class Compiler {
 public:
  // introducing railgun to this scope
  typedef railgun::Instruction Instruction;
  typedef railgun::OP OP;

  typedef std::unordered_map<railgun::Code*, std::string> EntryPointMap;
  typedef std::vector<railgun::Code*> Codes;

  explicit Compiler(Context* ctx, railgun::Code* code, llvm::Module* module)
    : ctx_(ctx),
      code_(code),
      codes_(),
      module_(module)
  { }

  ~Compiler() {

    llvm::ExecutionEngine *EE = llvm::EngineBuilder(module_).create();
    llvm::EngineBuilder(module_).create();
    llvm::Function *F;
    // link entry points
    for (const auto& pair : entry_points_) {
      F = module_->getFunction(llvm::StringRef(pair.second));
      pair.first->set_executable(EE->getPointerToFunction(F));
    }
  }

  static inline std::string MakeBlockName(railgun::Code* code) {
    // example. Block 0x7fff5fbff8b812
    char ptr[25];
    sprintf(ptr,"Block %p", code);
    return std::string(ptr);
  }

  void Initialize(railgun::Code* code) {
    code_ = code;
    codes_.push_back(code);
    entry_points_.insert(std::make_pair(code, MakeBlockName(code)));
  }

  void Compile(railgun::Code* code) {
    Initialize(code);
    Main();
  }

  void Main() {
    namespace r = railgun;

    const Instruction* instr = code_->begin();
    for (const Instruction* last = code_->end(); instr != last;) {
        const uint32_t opcode = instr->GetOP();
      switch (opcode) {
        case r::OP::NOP:
          CompileNOP(instr);
          break;
        case r::OP::ENTER:
          CompileENTER(instr);
          break;
        case r::OP::MV:
          CompileMV(instr);
          break;
        case r::OP::UNARY_POSITIVE:
          CompileUNARY_POSITIVE(instr);
          break;
        case r::OP::UNARY_NEGATIVE:
          CompileUNARY_NEGATIVE(instr);
          break;
        case r::OP::UNARY_NOT:
          CompileUNARY_NOT(instr);
          break;
        case r::OP::UNARY_BIT_NOT:
          CompileUNARY_BIT_NOT(instr);
          break;
        case r::OP::BINARY_ADD:
          CompileBINARY_ADD(instr);
          break;
        case r::OP::BINARY_SUBTRACT:
          CompileBINARY_SUBTRACT(instr);
          break;
        case r::OP::BINARY_MULTIPLY:
          CompileBINARY_MULTIPLY(instr);
          break;
        case r::OP::BINARY_DIVIDE:
          CompileBINARY_DIVIDE(instr);
          break;
        case r::OP::BINARY_MODULO:
          CompileBINARY_MODULO(instr);
          break;
        case r::OP::BINARY_LSHIFT:
          CompileBINARY_LSHIFT(instr);
          break;
        case r::OP::BINARY_RSHIFT:
          CompileBINARY_RSHIFT(instr);
          break;
        case r::OP::BINARY_RSHIFT_LOGICAL:
          CompileBINARY_RSHIFT_LOGICAL(instr);
          break;
        case r::OP::BINARY_LT:
          CompileBINARY_LT(instr);
          break;
        case r::OP::BINARY_LTE:
          CompileBINARY_LTE(instr);
          break;
        case r::OP::BINARY_GT:
          CompileBINARY_GT(instr);
          break;
        case r::OP::BINARY_GTE:
          CompileBINARY_GTE(instr);
          break;
        case r::OP::BINARY_INSTANCEOF:
          CompileBINARY_INSTANCEOF(instr);
          break;
        case r::OP::BINARY_IN:
          CompileBINARY_IN(instr);
          break;
        case r::OP::BINARY_EQ:
          CompileBINARY_EQ(instr);
          break;
        case r::OP::BINARY_STRICT_EQ:
          CompileBINARY_STRICT_EQ(instr);
          break;
        case r::OP::BINARY_NE:
          CompileBINARY_NE(instr);
          break;
        case r::OP::BINARY_STRICT_NE:
          CompileBINARY_STRICT_NE(instr);
          break;
        case r::OP::BINARY_BIT_AND:
          CompileBINARY_BIT_AND(instr);
          break;
        case r::OP::BINARY_BIT_XOR:
          CompileBINARY_BIT_XOR(instr);
          break;
        case r::OP::BINARY_BIT_OR:
          CompileBINARY_BIT_OR(instr);
          break;
        case r::OP::RETURN:
          CompileRETURN(instr);
          break;
        case r::OP::THROW:
          CompileTHROW(instr);
          break;
        case r::OP::POP_ENV:
          CompilePOP_ENV(instr);
          break;
        case r::OP::WITH_SETUP:
          CompileWITH_SETUP(instr);
          break;
        case r::OP::RETURN_SUBROUTINE:
          CompileRETURN_SUBROUTINE(instr);
          break;
        case r::OP::DEBUGGER:
          CompileDEBUGGER(instr);
          break;
        case r::OP::LOAD_REGEXP:
          CompileLOAD_REGEXP(instr);
          break;
        case r::OP::LOAD_OBJECT:
          CompileLOAD_OBJECT(instr);
          break;
        case r::OP::LOAD_ELEMENT:
          CompileLOAD_ELEMENT(instr);
          break;
        case r::OP::STORE_ELEMENT:
          CompileSTORE_ELEMENT(instr);
          break;
        case r::OP::DELETE_ELEMENT:
          CompileDELETE_ELEMENT(instr);
          break;
        case r::OP::INCREMENT_ELEMENT:
          CompileINCREMENT_ELEMENT(instr);
          break;
        case r::OP::DECREMENT_ELEMENT:
          CompileDECREMENT_ELEMENT(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_ELEMENT:
          CompilePOSTFIX_INCREMENT_ELEMENT(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_ELEMENT:
          CompilePOSTFIX_DECREMENT_ELEMENT(instr);
          break;
        case r::OP::TO_NUMBER:
          CompileTO_NUMBER(instr);
          break;
        case r::OP::TO_PRIMITIVE_AND_TO_STRING:
          CompileTO_PRIMITIVE_AND_TO_STRING(instr);
          break;
        case r::OP::CONCAT:
          CompileCONCAT(instr);
          break;
        case r::OP::RAISE:
          CompileRAISE(instr);
          break;
        case r::OP::TYPEOF:
          CompileTYPEOF(instr);
          break;
        case r::OP::STORE_OBJECT_INDEXED:
          CompileSTORE_OBJECT_INDEXED(instr);
          break;
        case r::OP::STORE_OBJECT_DATA:
          CompileSTORE_OBJECT_DATA(instr);
          break;
        case r::OP::STORE_OBJECT_GET:
          CompileSTORE_OBJECT_GET(instr);
          break;
        case r::OP::STORE_OBJECT_SET:
          CompileSTORE_OBJECT_SET(instr);
          break;
        case r::OP::LOAD_PROP:
          CompileLOAD_PROP(instr);
          break;
        case r::OP::LOAD_PROP_GENERIC:
        case r::OP::LOAD_PROP_OWN:
        case r::OP::LOAD_PROP_PROTO:
        case r::OP::LOAD_PROP_CHAIN:
          UNREACHABLE();
          break;
        case r::OP::STORE_PROP:
          CompileSTORE_PROP(instr);
          break;
        case r::OP::STORE_PROP_GENERIC:
          UNREACHABLE();
          break;
        case r::OP::DELETE_PROP:
          CompileDELETE_PROP(instr);
          break;
        case r::OP::INCREMENT_PROP:
          CompileINCREMENT_PROP(instr);
          break;
        case r::OP::DECREMENT_PROP:
          CompileDECREMENT_PROP(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_PROP:
          CompilePOSTFIX_INCREMENT_PROP(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_PROP:
          CompilePOSTFIX_DECREMENT_PROP(instr);
          break;
        case r::OP::LOAD_CONST:
          CompileLOAD_CONST(instr);
          break;
        case r::OP::JUMP_BY:
          CompileJUMP_BY(instr);
          break;
        case r::OP::JUMP_SUBROUTINE:
          CompileJUMP_SUBROUTINE(instr);
          break;
        case r::OP::IF_FALSE:
          CompileIF_FALSE(instr);
          break;
        case r::OP::IF_TRUE:
          CompileIF_TRUE(instr);
          break;
        case r::OP::IF_FALSE_BINARY_LT:
          CompileBINARY_LT(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_LT:
          CompileBINARY_LT(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_LTE:
          CompileBINARY_LTE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_LTE:
          CompileBINARY_LTE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_GT:
          CompileBINARY_GT(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_GT:
          CompileBINARY_GT(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_GTE:
          CompileBINARY_GTE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_GTE:
          CompileBINARY_GTE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_INSTANCEOF:
          CompileBINARY_INSTANCEOF(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_INSTANCEOF:
          CompileBINARY_INSTANCEOF(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_IN:
          CompileBINARY_IN(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_IN:
          CompileBINARY_IN(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_EQ:
          CompileBINARY_EQ(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_EQ:
          CompileBINARY_EQ(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_NE:
          CompileBINARY_NE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_NE:
          CompileBINARY_NE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_STRICT_EQ:
          CompileBINARY_STRICT_EQ(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_STRICT_EQ:
          CompileBINARY_STRICT_EQ(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_STRICT_NE:
          CompileBINARY_STRICT_NE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_STRICT_NE:
          CompileBINARY_STRICT_NE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_BIT_AND:
          CompileBINARY_BIT_AND(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_BIT_AND:
          CompileBINARY_BIT_AND(instr, OP::IF_TRUE);
          break;
        case r::OP::FORIN_SETUP:
          CompileFORIN_SETUP(instr);
          break;
        case r::OP::FORIN_ENUMERATE:
          CompileFORIN_ENUMERATE(instr);
          break;
        case r::OP::FORIN_LEAVE:
          CompileFORIN_LEAVE(instr);
          break;
        case r::OP::TRY_CATCH_SETUP:
          CompileTRY_CATCH_SETUP(instr);
          break;
        case r::OP::LOAD_NAME:
          CompileLOAD_NAME(instr);
          break;
        case r::OP::STORE_NAME:
          CompileSTORE_NAME(instr);
          break;
        case r::OP::DELETE_NAME:
          CompileDELETE_NAME(instr);
          break;
        case r::OP::INCREMENT_NAME:
          CompileINCREMENT_NAME(instr);
          break;
        case r::OP::DECREMENT_NAME:
          CompileDECREMENT_NAME(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_NAME:
          CompilePOSTFIX_INCREMENT_NAME(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_NAME:
          CompilePOSTFIX_DECREMENT_NAME(instr);
          break;
        case r::OP::TYPEOF_NAME:
          CompileTYPEOF_NAME(instr);
          break;
        case r::OP::LOAD_GLOBAL:
          CompileLOAD_GLOBAL(instr);
          break;
        case r::OP::LOAD_GLOBAL_DIRECT:
          CompileLOAD_GLOBAL_DIRECT(instr);
          break;
        case r::OP::STORE_GLOBAL:
          CompileSTORE_GLOBAL(instr);
          break;
        case r::OP::STORE_GLOBAL_DIRECT:
          CompileSTORE_GLOBAL_DIRECT(instr);
          break;
        case r::OP::DELETE_GLOBAL:
          CompileDELETE_GLOBAL(instr);
          break;
        case r::OP::TYPEOF_GLOBAL:
          CompileTYPEOF_GLOBAL(instr);
          break;
        case r::OP::LOAD_HEAP:
          CompileLOAD_HEAP(instr);
          break;
        case r::OP::STORE_HEAP:
          CompileSTORE_HEAP(instr);
          break;
        case r::OP::DELETE_HEAP:
          CompileDELETE_HEAP(instr);
          break;
        case r::OP::INCREMENT_HEAP:
          CompileINCREMENT_HEAP(instr);
          break;
        case r::OP::DECREMENT_HEAP:
          CompileDECREMENT_HEAP(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_HEAP:
          CompilePOSTFIX_INCREMENT_HEAP(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_HEAP:
          CompilePOSTFIX_DECREMENT_HEAP(instr);
          break;
        case r::OP::TYPEOF_HEAP:
          CompileTYPEOF_HEAP(instr);
          break;
        case r::OP::INCREMENT:
          CompileINCREMENT(instr);
          break;
        case r::OP::DECREMENT:
          CompileDECREMENT(instr);
          break;
        case r::OP::POSTFIX_INCREMENT:
          CompilePOSTFIX_INCREMENT(instr);
          break;
        case r::OP::POSTFIX_DECREMENT:
          CompilePOSTFIX_DECREMENT(instr);
          break;
        case r::OP::PREPARE_DYNAMIC_CALL:
          CompilePREPARE_DYNAMIC_CALL(instr);
          break;
        case r::OP::CALL:
          CompileCALL(instr);
          break;
        case r::OP::CONSTRUCT:
          CompileCONSTRUCT(instr);
          break;
        case r::OP::EVAL:
          CompileEVAL(instr);
          break;
        case r::OP::RESULT:
          CompileRESULT(instr);
          break;
        case r::OP::LOAD_FUNCTION:
          CompileLOAD_FUNCTION(instr);
          break;
        case r::OP::INIT_VECTOR_ARRAY_ELEMENT:
          CompileINIT_VECTOR_ARRAY_ELEMENT(instr);
          break;
        case r::OP::INIT_SPARSE_ARRAY_ELEMENT:
          CompileINIT_SPARSE_ARRAY_ELEMENT(instr);
          break;
        case r::OP::LOAD_ARRAY:
          CompileLOAD_ARRAY(instr);
          break;
        case r::OP::DUP_ARRAY:
          CompileDUP_ARRAY(instr);
          break;
        case r::OP::BUILD_ENV:
          CompileBUILD_ENV(instr);
          break;
        case r::OP::INSTANTIATE_DECLARATION_BINDING:
          CompileINSTANTIATE_DECLARATION_BINDING(instr);
          break;
        case r::OP::INSTANTIATE_VARIABLE_BINDING:
          CompileINSTANTIATE_VARIABLE_BINDING(instr);
          break;
        case r::OP::INITIALIZE_HEAP_IMMUTABLE:
          CompileINITIALIZE_HEAP_IMMUTABLE(instr);
          break;
        case r::OP::LOAD_ARGUMENTS:
          CompileLOAD_ARGUMENTS(instr);
          break;
      }
      const uint32_t length = r::kOPLength[opcode];
      std::advance(instr, length);
    }
  }

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
  void CompileBINARY_LT(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_LTE(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_GT(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_GTE(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileCompare(const Instruction* instr, OP::Type fused);
  void CompileBINARY_INSTANCEOF(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_IN(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_EQ(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_STRICT_EQ(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_NE(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_STRICT_NE(const Instruction* instr, OP::Type fused = OP::NOP);
  void CompileBINARY_BIT_AND(const Instruction* instr, OP::Type fused = OP::NOP);
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

  Context* ctx_;
  railgun::Code* code_;
  EntryPointMap entry_points_;
  Codes codes_;
  llvm::Module *module_;
};

} } }  // namespace iv::lv5::diagram