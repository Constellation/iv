#ifndef _IV_LV5_RUNTIME_GLOBAL_H_
#define _IV_LV5_RUNTIME_GLOBAL_H_
#include <cmath>
#include <tr1/memory>
#include "arguments.h"
#include "jsval.h"
#include "jsstring.h"
#include "error.h"
#include "context.h"
#include "factory.h"
#include "eval_source.h"
#include "parser.h"
#include "lv5.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

inline JSScript* CompileScript(Context* ctx, JSString* str, Error* error) {
  std::tr1::shared_ptr<EvalSource> const src(new EvalSource(str));
  AstFactory* const factory = new AstFactory(ctx);
  core::Parser<AstFactory, EvalSource> parser(factory, src.get());
  parser.set_strict(ctx->IsStrict());
  const iv::lv5::FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    delete factory;
    error->Report(Error::Syntax,
                  parser.error());
    return NULL;
  } else {
    return iv::lv5::JSEvalScript<EvalSource>::New(ctx, eval, factory, src);
  }
}

}  // namespace iv::lv5::runtime::detail

inline JSVal InDirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = detail::CompileScript(args.ctx(),
                                                 first.string(), ERROR(error));
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

inline JSVal GlobalEval(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("eval", args, error);
  return InDirectCallToEval(args, error);
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
  JSScript* const script = detail::CompileScript(args.ctx(),
                                                 first.string(), ERROR(error));
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

inline JSVal GlobalParseInt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("parseInt", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    int radix = 0;
    if (args.size() > 1) {
      const double ret = args[1].ToNumber(args.ctx(), ERROR(error));
      radix = core::DoubleToInt32(ret);
    }
    bool strip_prefix = true;
    if (radix != 0) {
      if (radix < 2 || radix > 36) {
        return JSNaN;
      }
      if (radix != 16) {
        strip_prefix = false;
      }
    } else {
      radix = 10;
    }
    return core::StringToIntegerWithRadix(str->begin(), str->end(),
                                          radix,
                                          strip_prefix);
  } else {
    return JSNaN;
  }
}

inline JSVal GlobalParseFloat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("parseFloat", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    return core::StringToDouble(str->value(), true);
  } else {
    return JSNaN;
  }
}

inline JSVal GlobalIsNaN(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("isNaN", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(number)) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSTrue;
  }
}

inline JSVal GlobalIsFinite(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("isFinite", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), ERROR(error));
    if (std::isfinite(number)) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSTrue;
  }
}

inline JSVal ThrowTypeError(const Arguments& args, Error* error) {
  error->Report(Error::Type,
                "[[ThrowTypeError]] called");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_GLOBAL_H_
