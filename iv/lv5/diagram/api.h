#ifndef IV_LV5_DIAGRAM_API_H_
#define IV_LV5_DIAGRAM_API_H_
#include <iv/lv5/breaker/context.h>
#include <iv/lv5/diagram/compiler.h>
namespace iv {
namespace lv5 {
namespace diagram {

inline void ExecuteInGlobal(breaker::Context* ctx,
                            std::shared_ptr<core::FileSource> src, Error* e) {
  railgun::Code* code =
      railgun::CompileInGlobal(ctx, src, true, IV_LV5_ERROR_VOID(e));
  Compile(ctx, code);
  Run(ctx, code, e);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_DIAGRAM_API_H_
