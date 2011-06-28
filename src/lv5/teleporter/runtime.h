#ifndef _IV_LV5_TELEPORTER_RUNTIME_H_
#define _IV_LV5_TELEPORTER_RUNTIME_H_
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
#include "lv5/teleporter/fwd.h"
#include "lv5/teleporter/context.h"
#include "lv5/teleporter/utility.h"
#include "lv5/internal.h"
#include "lv5/json.h"
namespace iv {
namespace lv5 {
namespace teleporter {

// GlobalEval is InDirectCallToEval
inline JSVal GlobalEval(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("eval", args, e);
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  JSString* str = first.string();
  Context* const ctx = static_cast<Context*>(args.ctx());
  // if str is (...) expression,
  // parse as JSON (RejectLineTerminator Pattern) at first
  if (str->size() > 2 &&
      (*str)[0] == '(' &&
      (*str)[str->size() - 1] == ')') {
    Error json_parse_error;
    const JSVal result = ParseJSON<false>(ctx,
                                          core::UStringPiece(str->data() + 1,
                                                             str->size() - 2),
                                          &json_parse_error);
    if (!json_parse_error) {
      return result;
    }
  }
  JSScript* const script = CompileScript(ctx, str,
                                         false, IV_LV5_ERROR(e));
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
  IV_LV5_CONSTRUCTOR_CHECK("eval", args, e);
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  JSString* str = first.string();
  Context* const ctx = static_cast<Context*>(args.ctx());
  // if str is (...) expression,
  // parse as JSON (RejectLineTerminator Pattern) at first
  if (str->size() > 2 &&
      (*str)[0] == '(' &&
      (*str)[str->size() - 1] == ')' &&
      !ctx->IsStrict()) {
    Error json_parse_error;
    const JSVal result = ParseJSON<false>(ctx,
                                          core::UStringPiece(str->data() + 1,
                                                             str->size() - 2),
                                          &json_parse_error);
    if (!json_parse_error) {
      return result;
    }
  }
  JSScript* const script = CompileScript(ctx, str,
                                         ctx->IsStrict(), IV_LV5_ERROR(e));
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
  internal::BuildFunctionSource(&builder, args, IV_LV5_ERROR(e));
  JSString* const source = builder.Build(ctx);
  JSScript* const script = CompileScript(ctx, source, false, IV_LV5_ERROR(e));
  internal::IsOneFunctionExpression(*script->function(), IV_LV5_ERROR(e));
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

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_RUNTIME_H_
