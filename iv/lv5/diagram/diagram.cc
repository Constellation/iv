// diagram entry points
//
#include <mutex>
#include <llvm/Support/TargetSelect.h>

#include <iv/lv5/diagram/api.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/context.h>
#include <iv/lv5/breaker/entry_point.h>
#include <iv/lv5/diagram/compiler.h>
namespace iv {
namespace lv5 {
namespace diagram {

static std::once_flag flag;

void Initialize() {
  std::call_once(flag, []() {
    // Initialize LLVM
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
  });
}

void ExecuteInGlobal(breaker::Context* ctx,
                     std::shared_ptr<core::FileSource> src, Error* e) {
  railgun::Code* code =
      railgun::CompileInGlobal(ctx, src, true, IV_LV5_ERROR_VOID(e));
  diagram::Compile(ctx, code);
  breaker::Run(ctx, code, e);
}

} } } // namespace iv::lv5::diagram
