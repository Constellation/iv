#ifndef _IV_LV5_RUNTIME_RAILGUN_H_
#define _IV_LV5_RUNTIME_RAILGUN_H_
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
#include "lv5/internal.h"
#include "lv5/json.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/utility.h"
#include "lv5/railgun/jsfunction.h"
namespace iv {
namespace lv5 {
namespace railgun {

inline JSVal FunctionConstructor(const Arguments& args, Error* e) {
  Context* const ctx = static_cast<Context*>(args.ctx());
  StringBuilder builder;
  internal::BuildFunctionSource(&builder, args, IV_LV5_ERROR(e));
  JSString* const source = builder.Build(ctx);
  Code* const code = CompileFunction(ctx, source, false, true, IV_LV5_ERROR(e));
  return JSVMFunction::New(ctx, code, ctx->global_env());
}

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

  std::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory factory(ctx);
  core::Parser<AstFactory, EvalSource> parser(&factory, *src);
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    e->Report(Error::Syntax, parser.error());
    return JSUndefined;
  }
//  JSScript* script = JSEvalScript<EvalSource>::New(ctx, src);
//  Code* code = Compile(ctx, *eval, script);
  return JSUndefined;
//  if (eval->strict()) {
//    JSDeclEnv* const env =
//        internal::NewDeclarativeEnvironment(ctx, ctx->global_env());
//    const ContextSwitcher switcher(ctx,
//                                   env,
//                                   env,
//                                   ctx->global_obj(),
//                                   true);
//    ctx->Run(script);
//  } else {
//    const ContextSwitcher switcher(ctx,
//                                   ctx->global_env(),
//                                   ctx->global_env(),
//                                   ctx->global_obj(),
//                                   false);
//    ctx->Run(script);
//  }
//  return ctx->ret();
}

inline JSVal DirectCallToEval(const Arguments& args, Error* e) {
  return JSUndefined;
}

} } }  // iv::lv5::railgun
#endif  // _IV_LV5_RUNTIME_RAILGUN_H_
