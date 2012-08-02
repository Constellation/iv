#ifndef IV_LV5_TELEPORTER_RUNTIME_H_
#define IV_LV5_TELEPORTER_RUNTIME_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/teleporter/fwd.h>
#include <iv/lv5/teleporter/context.h>
#include <iv/lv5/teleporter/utility.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/json.h>
namespace iv {
namespace lv5 {
namespace teleporter {

// GlobalEval is InDirectCallToEval
inline JSVal GlobalEval(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("eval", args, e);
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal first = args[0];
  if (!first.IsString()) {
    return first;
  }
  JSString* str = first.string();
  Context* const ctx = static_cast<Context*>(args.ctx());
  // if str is (...) expression,
  // parse as JSON (RejectLineTerminator Pattern) at first
  if (str->size() > 2) {
    MaybeJSONParser maybe(str);
    if (maybe.IsParsable()) {
      Error::Dummy json_parse_error;
      const JSVal res = maybe.Parse(ctx, &json_parse_error);
      if (!json_parse_error) {
        return res;
      }
    }
  }
  JSScript* const script = CompileScript(ctx, str,
                                         false, IV_LV5_ERROR(e));
  if (script->function()->strict()) {
    JSDeclEnv* const env = JSDeclEnv::New(ctx, ctx->global_env());
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
  const JSVal first = args[0];
  if (!first.IsString()) {
    return first;
  }
  JSString* str = first.string();
  Context* const ctx = static_cast<Context*>(args.ctx());
  // if str is (...) expression,
  // parse as JSON (RejectLineTerminator Pattern) at first
  if (str->size() > 2 && !ctx->IsStrict()) {
    MaybeJSONParser maybe(str);
    if (maybe.IsParsable()) {
      Error::Dummy json_parse_error;
      const JSVal res = maybe.Parse(ctx, &json_parse_error);
      if (!json_parse_error) {
        return res;
      }
    }
  }
  JSScript* const script = CompileScript(ctx, str,
                                         ctx->IsStrict(), IV_LV5_ERROR(e));
  if (script->function()->strict()) {
    JSDeclEnv* const env = JSDeclEnv::New(ctx, ctx->lexical_env());
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
  JSStringBuilder builder;
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
#endif  // IV_LV5_TELEPORTER_RUNTIME_H_
