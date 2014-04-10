#ifndef IV_LV5_DIAGRAM_COMPILER_H_
#define IV_LV5_DIAGRAM_COMPILER_H_
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/LLVMContext.h>

#include <iv/debug.h>
#include <iv/byteorder.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/compiler.h>
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

  breaker::Context* ctx_;
  railgun::Code* code_;
  EntryPointMap entry_points_;
  Codes codes_;
  llvm::Module *module_;
};

inline void CompileInternal(Compiler* diagram, breaker::Compiler* breaker,
                            railgun::Code* code) {
  Compiler::CompileStatus status =  diagram->Compile(code);
  if (status != Compiler::CompileStatus_Compiled) {
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
