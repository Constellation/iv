#ifndef IV_LV5_DIAGRAM_COMPILER_H_
#define IV_LV5_DIAGRAM_COMPILER_H_
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ADT/StringRef.h>

#include <iv/debug.h>
#include <iv/byteorder.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/railgun.h>
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

  void Main();

  Context* ctx_;
  railgun::Code* code_;
  EntryPointMap entry_points_;
  Codes codes_;
  llvm::Module *module_;
};

} } }  // namespace iv::lv5::diagram
#endif  // IV_LV5_DIAGRAM_COMPILER_H_
