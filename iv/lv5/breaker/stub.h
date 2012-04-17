// Stub functions implementations
#ifndef IV_LV5_BREAKER_STUB_H_
#define IV_LV5_BREAKER_STUB_H_
namespace iv {
namespace lv5 {
namespace breaker {
namespace stub {

JSVal RETURN(JSVal val, railgun::Frame* frame) {
  if ((frame->constructor_call_ && !val.IsObject())) {
    val = frame->GetThis();
  }
  // because of Frame is code frame,
  // first lexical_env is variable_env.
  // (if Eval / Global, this is not valid)
  assert(frame->lexical_env() == frame->variable_env());
  stack_.Unwind(frame);
  return val;
}

} } } }  // namespace iv::lv5::breaker::stub
#endif  // IV_LV5_BREAKER_STUB_H_
