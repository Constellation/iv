#ifndef IV_LV5_RAILGUN_RUNTIME_H_
#define IV_LV5_RAILGUN_RUNTIME_H_
#include <iv/detail/memory.h>
#include <iv/parser.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/json.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/frame.h>
#include <iv/lv5/railgun/jsfunction.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline JSVal FunctionConstructor(const Arguments& args, Error* e) {
  Context* const ctx = static_cast<Context*>(args.ctx());
  JSStringBuilder builder;
  internal::BuildFunctionSource(&builder, args, IV_LV5_ERROR(e));
  const JSString* str = builder.Build(ctx);
  std::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory factory(ctx);
  core::Parser<AstFactory, EvalSource> parser(&factory,
                                              *src, ctx->symbol_table());
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    e->Report(Error::Syntax, parser.error());
    return JSEmpty;
  }
  const FunctionLiteral* const func =
      internal::IsOneFunctionExpression(*eval, IV_LV5_ERROR(e));
  JSScript* script = JSSourceScript<EvalSource>::New(ctx, src);
  Code* code = CompileFunction(ctx, *func, script);
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
  if (str->size() > 2) {
    MaybeJSONParser maybe(str);
    if (maybe.IsParsable()) {
      Error json_parse_error;
      const JSVal res = maybe.Parse(ctx, &json_parse_error);
      if (!json_parse_error) {
        return res;
      }
    }
  }

  std::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory factory(ctx);
  core::Parser<AstFactory, EvalSource> parser(&factory, *src,
                                              ctx->symbol_table());
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    e->Report(Error::Syntax, parser.error());
    return JSUndefined;
  }
  JSScript* script = JSSourceScript<EvalSource>::New(ctx, src);
  Code* code = CompileIndirectEval(ctx, *eval, script);
  return ctx->vm()->RunEval(
      code,
      ctx->global_env(),
      ctx->global_env(),
      ctx->global_obj(), e);
}

inline JSVal DirectCallToEval(const Arguments& args, Frame* frame, Error* e) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  JSString* str = first.string();
  Context* const ctx = static_cast<Context*>(args.ctx());
  const bool strict = frame->code()->strict();
  // if str is (...) expression,
  // parse as JSON (RejectLineTerminator Pattern) at first
  if (str->size() > 2 && !strict) {
    MaybeJSONParser maybe(str);
    if (maybe.IsParsable()) {
      Error json_parse_error;
      const JSVal res = maybe.Parse(ctx, &json_parse_error);
      if (!json_parse_error) {
        return res;
      }
    }
  }

  Code* code = ctx->direct_eval_map()->Lookup(str);

  if (!code) {
    std::shared_ptr<EvalSource> const src(new EvalSource(*str));
    AstFactory factory(ctx);
    core::Parser<AstFactory, EvalSource> parser(&factory, *src,
                                                ctx->symbol_table());
    parser.set_strict(strict);
    const FunctionLiteral* const eval = parser.ParseProgram();
    if (!eval) {
      e->Report(Error::Syntax, parser.error());
      return JSUndefined;
    }
    JSScript* script = JSSourceScript<EvalSource>::New(ctx, src);
    code = CompileEval(ctx, *eval, script);
    if (!code->strict()) {
      ctx->direct_eval_map()->Insert(str, code);
    }
  }

  VM* const vm = ctx->vm();
  return vm->RunEval(
      code,
      vm->stack()->current()->variable_env(),
      vm->stack()->current()->lexical_env(),
      vm->stack()->current()->GetThis(), e);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_RUNTIME_H_
