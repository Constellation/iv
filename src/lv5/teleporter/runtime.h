#ifndef _IV_LV5_RUNTIME_TELEPORTER_H_
#define _IV_LV5_RUNTIME_TELEPORTER_H_
#include "lv5/lv5.h"
#include "lv5/teleporter/fwd.h"
#include "lv5/teleporter/context.h"
#include "lv5/teleporter/utility.h"
#include "lv5/internal.h"
namespace iv {
namespace lv5 {
namespace teleporter {

// GlobalEval is InDirectCallToEval
inline JSVal GlobalEval(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("eval", args, e);
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = static_cast<Context*>(args.ctx());
  JSScript* const script = CompileScript(ctx, first.string(), false, ERROR(e));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        internal::NewDeclarativeEnvironment(ctx, ctx->global_env());
    const ContextSwitcher switcher(ctx,
                                   env,
                                   env,
                                   ctx->global_obj(),
                                   true);
    ctx->Run(script);
  } else {
    const ContextSwitcher switcher(ctx,
                                   ctx->global_env(),
                                   ctx->global_env(),
                                   ctx->global_obj(),
                                   false);
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

inline JSVal DirectCallToEval(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("eval", args, e);
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = static_cast<Context*>(args.ctx());
  JSScript* const script = CompileScript(ctx, first.string(),
                                         ctx->IsStrict(), ERROR(e));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        internal::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const ContextSwitcher switcher(ctx,
                                   env,
                                   env,
                                   ctx->this_binding(),
                                   true);
    ctx->Run(script);
  } else {
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

inline JSVal FunctionConstructor(const Arguments& args, Error* e) {
  Context* const ctx = static_cast<Context*>(args.ctx());
  StringBuilder builder;
  internal::BuildFunctionSource(&builder, args, ERROR(e));
  JSString* const source = builder.Build(ctx);
  JSScript* const script = CompileScript(ctx, source, false, ERROR(e));
  internal::IsOneFunctionExpression(*script->function(), ERROR(e));
  const ContextSwitcher switcher(ctx,
                                 ctx->global_env(),
                                 ctx->global_env(),
                                 ctx->global_obj(),
                                 false);
  ctx->Run(script);
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

} } }  // iv::lv5::teleporter
#endif  // _IV_LV5_RUNTIME_TELEPORTER_H_
