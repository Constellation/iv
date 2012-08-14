#ifndef IV_LV5_BREAKER_CONTEXT_H_
#define IV_LV5_BREAKER_CONTEXT_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/context_fwd.h>
#include <iv/lv5/breaker/jsfunction.h>
namespace iv {
namespace lv5 {
namespace breaker {

inline lv5::JSFunction* Context::NewFunction(railgun::Code* code, JSEnv* env) {
  return breaker::JSFunction::New(this, code, env);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_CONTEXT_H_
