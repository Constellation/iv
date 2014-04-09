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
namespace iv {
namespace lv5 {
namespace diagram {

class Compiler {
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

  void Main();

 public:
  explicit Compiler(Context* ctx, railgun::Code* code)
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

  void Compile(railgun::Code* code) {
    Initialize(code);
    Main();
  }

 private:
  Context* ctx_;
  railgun::Code* code_;
  EntryPointMap entry_points_;
  Codes codes_;
  llvm::Module *module_;
};

inline void CompileInternal(Compiler* compiler, railgun::Code* code) {
  compiler->Compile(code);
  for (railgun::Code* sub : code->codes()) {
    CompileInternal(compiler, sub);
  }
}

inline void Compile(Context* ctx, railgun::Code* code) {
  Compiler compiler(ctx, code);
  CompileInternal(&compiler, code);
}

} } }  // namespace iv::lv5::diagram
#endif  // IV_LV5_DIAGRAM_COMPILER_H_
