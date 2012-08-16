#ifndef IV_LV5_BREAKER_API_H_
#define IV_LV5_BREAKER_API_H_
#include <iv/lv5/breaker/context.h>
namespace iv {
namespace lv5 {
namespace breaker {

inline void ExecuteInGlobal(Context* ctx,
                            std::shared_ptr<core::FileSource> src, Error* e) {
  railgun::Code* code =
      railgun::CompileInGlobal(ctx, src, true, IV_LV5_ERROR_VOID(e));
  Compile(ctx, code);
  Run(ctx, code, e);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_API_H_
