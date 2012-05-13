#ifndef IV_LV5_BREAKER_ENTRY_POINT_H_
#define IV_LV5_BREAKER_ENTRY_POINT_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
namespace iv {
namespace lv5 {
namespace breaker {

inline JSVal RunEval(railgun::Context* ctx,
                     railgun::Code* code,
                     JSEnv* variable_env,
                     JSEnv* lexical_env,
                     JSVal this_binding) {
  Error* e = ctx->PendingError();
  ScopedArguments args(ctx, 0, IV_LV5_ERROR(e));
  args.set_this_binding(this_binding);
  railgun::Frame* frame = ctx->vm()->stack()->NewEvalFrame(
      ctx,
      args.ExtractBase(),
      code,
      variable_env,
      lexical_env);
  if (!frame) {
    e->Report(Error::Range, "maximum call stack size exceeded");
    return JSEmpty;
  }
  frame->InitThisBinding(ctx, e);
  if (*e) {
    ctx->vm()->stack()->Unwind(frame);
    return JSEmpty;
  }
  const JSVal res = breaker_prologue(ctx, frame, code->executable());
#ifdef DEBUG
  if (code->needs_declarative_environment()) {
    assert(frame->lexical_env()->outer() == lexical_env);
  } else {
    assert(frame->lexical_env() == lexical_env);
  }
#endif
  ctx->vm()->stack()->Unwind(frame);
  return res;
}

inline JSVal Run(railgun::Context* ctx, railgun::Code* code) {
  return RunEval(ctx,
                 code,
                 ctx->global_env(),
                 ctx->global_env(),
                 ctx->global_obj());
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_ENTRY_POINT_H_
