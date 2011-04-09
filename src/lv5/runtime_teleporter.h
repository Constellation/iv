#ifndef _IV_LV5_RUNTIME_TELEPORTER_H_
#define _IV_LV5_RUNTIME_TELEPORTER_H_
#include "lv5/teleporter.h"
namespace iv {
namespace lv5 {
namespace teleporter {
namespace detail {

template<typename Builder>
void BuildFunctionSource(Builder* builder, const Arguments& args, Error* e) {
  const std::size_t arg_count = args.size();
  Context* const ctx = args.ctx();
  if (arg_count == 0) {
    builder->append("(function() { \n})");
  } else if (arg_count == 1) {
    builder->append("(function() { ");
    JSString* const str = args[0].ToString(ctx, ERROR_VOID(e));
    builder->append(*str);
    builder->append("\n})");
  } else {
    builder->append("(function(");
    Arguments::const_iterator it = args.begin();
    const Arguments::const_iterator last = args.end() - 1;
    do {
      JSString* const str = it->ToString(ctx, ERROR_VOID(e));
      builder->append(*str);
      ++it;
      if (it != last) {
        builder->push_back(',');
      } else {
        break;
      }
    } while (true);
    builder->append(") { ");
    JSString* const prog = last->ToString(ctx, ERROR_VOID(e));
    builder->append(*prog);
    builder->append("\n})");
  }
}

inline void CheckFunctionExpressionIsOne(const FunctionLiteral& func,
                                         Error* e) {
  const FunctionLiteral::Statements& stmts = func.body();
  if (stmts.size() == 1) {
    const Statement& stmt = *stmts[0];
    if (stmt.AsExpressionStatement()) {
      if (stmt.AsExpressionStatement()->expr()->AsFunctionLiteral()) {
        return;
      }
    }
  }
  e->Report(Error::Syntax,
            "Function Constructor with invalid arguments");
}

}  // namespace iv::lv5::teleporter::detail

inline JSVal InDirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(), first.string(),
                                         false, ERROR(error));
                                         //  ctx->IsStrict(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        Interpreter::NewDeclarativeEnvironment(ctx, ctx->global_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->global_obj(),
                                                true);
    ctx->Run(script);
  } else {
    const Interpreter::ContextSwitcher switcher(ctx,
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

inline JSVal DirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(), first.string(),
                                         ctx->IsStrict(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        Interpreter::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const Interpreter::ContextSwitcher switcher(ctx,
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
  Context* const ctx = args.ctx();
  StringBuilder builder;
  detail::BuildFunctionSource(&builder, args, ERROR(e));
  JSString* const source = builder.Build(ctx);
  JSScript* const script = CompileScript(ctx, source, false, ERROR(e));
  detail::CheckFunctionExpressionIsOne(*script->function(), ERROR(e));
  const Interpreter::ContextSwitcher switcher(ctx,
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

}  // iv::lv5::teleporter
namespace runtime {
} } }  // iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_TELEPORTER_H_
