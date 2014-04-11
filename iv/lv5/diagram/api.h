#ifndef IV_LV5_DIAGRAM_API_H_
#define IV_LV5_DIAGRAM_API_H_
#include <memory>
#include <iv/file_source.h>
#include <iv/lv5/error.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace diagram {

void Initialize();

void ExecuteInGlobal(breaker::Context* ctx,
                     std::shared_ptr<core::FileSource> src, Error* e);

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_DIAGRAM_API_H_
