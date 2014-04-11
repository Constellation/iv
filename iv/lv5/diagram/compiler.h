#ifndef IV_LV5_DIAGRAM_COMPILER_H_
#define IV_LV5_DIAGRAM_COMPILER_H_
#include <iv/debug.h>
#include <iv/byteorder.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/compiler.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
namespace iv {
namespace lv5 {
namespace diagram {

class Compiler {
 public:
  enum CompileStatus {
    CompileStatus_Error,
    CompileStatus_NotCompiled,
    CompileStatus_Compiled
  };

  explicit Compiler(breaker::Context* ctx, railgun::Code* code)
  : ctx_(ctx),
    code_(code),
    codes_() {
      module_ = new llvm::Module("null", llvm::getGlobalContext());
  }

  ~Compiler();

  CompileStatus Compile(railgun::Code* code) {
    Initialize(code);
    return Main();
  }

 private:
  // introducing railgun to this scope
  typedef railgun::Instruction Instruction;
  typedef railgun::OP OP;

  typedef std::unordered_map<railgun::Code*, std::string> EntryPointMap;
  typedef std::vector<railgun::Code*> Codes;

  static inline std::string MakeBlockName(railgun::Code* code) {
    // i.e. Block 0x7fff5fbff8b812
    char ptr[25];
    sprintf(ptr,"Block %p", code);
    return std::string(ptr);
  }

  void Initialize(railgun::Code* code) {
    code_ = code;
    codes_.push_back(code);
    entry_points_.insert(std::make_pair(code, MakeBlockName(code)));
  }

  CompileStatus Main();
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
                                 railgun::OP::Type fused);
  CompileStatus CompileBINARY_LTE(const Instruction* instr,
                                  railgun::OP::Type fused);
  CompileStatus CompileBINARY_GT(const Instruction* instr,
                                 railgun::OP::Type fused);
  CompileStatus CompileBINARY_GTE(const Instruction* instr,
                                  railgun::OP::Type fused);
  CompileStatus CompileCompare(const Instruction* instr, railgun::OP::Type fused);
  CompileStatus CompileBINARY_INSTANCEOF(const Instruction* instr,
                                         railgun::OP::Type fused);
  CompileStatus CompileBINARY_IN(const Instruction* instr,
                                 railgun::OP::Type fused);
  CompileStatus CompileBINARY_EQ(const Instruction* instr,
                                 railgun::OP::Type fused);
  CompileStatus CompileBINARY_STRICT_EQ(const Instruction* instr,
                                        railgun::OP::Type fused);
  CompileStatus CompileBINARY_NE(const Instruction* instr,
                                 railgun::OP::Type fused);
  CompileStatus CompileBINARY_STRICT_NE(const Instruction* instr,
                                        railgun::OP::Type fused);
  CompileStatus CompileBINARY_BIT_AND(const Instruction* instr,
                                      railgun::OP::Type fused);
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

  breaker::Context* ctx_;
  railgun::Code* code_;
  EntryPointMap entry_points_;
  Codes codes_;
  llvm::Module *module_;
};

inline void CompileInternal(Compiler* diagram, breaker::Compiler* breaker,
                            railgun::Code* code) {
  if (code->CanDiagram() && diagram->Compile(code) != Compiler::CompileStatus_Compiled) {
    printf("false\n");
    breaker->Compile(code);
  }
  for (railgun::Code* sub : code->codes()) {
    CompileInternal(diagram, breaker, sub);
  }
}

inline void Compile(breaker::Context* ctx, railgun::Code* code) {
  Compiler diagram(ctx, code);
  breaker::Compiler breaker(ctx, code);
  CompileInternal(&diagram, &breaker, code);
}

} } }  // namespace iv::lv5::diagram
#endif  // IV_LV5_DIAGRAM_COMPILER_H_
