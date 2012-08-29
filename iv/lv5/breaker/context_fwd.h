#ifndef IV_LV5_BREAKER_CONTEXT_FWD_H_
#define IV_LV5_BREAKER_CONTEXT_FWD_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Context : public railgun::Context {
 public:
  Context()
    : railgun::Context(FunctionConstructor, GlobalEval) {
  }

  virtual JSFunction* NewFunction(railgun::Code* code, JSEnv* env);
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_CONTEXT_FWD_H_
