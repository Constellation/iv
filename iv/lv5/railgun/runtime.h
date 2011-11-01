#ifndef IV_LV5_RAILGUN_RUNTIME_H_
#define IV_LV5_RAILGUN_RUNTIME_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/json.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/frame.h>
#include <iv/lv5/railgun/utility.h>
#include <iv/lv5/railgun/jsfunction.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline JSVal FunctionConstructor(const Arguments& args, Error* e) {
  Context* const ctx = static_cast<Context*>(args.ctx());
  JSStringBuilder builder;
  internal::BuildFunctionSource(&builder, args, IV_LV5_ERROR(e));
  Code* const code = CompileFunction(ctx, builder.Build(ctx), IV_LV5_ERROR(e));
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
    const JSString::Fiber* fiber = str->GetFiber();
    if ((*fiber)[0] == '(' &&
        (*fiber)[str->size() - 1] == ')') {
      Error json_parse_error;
      const JSVal result = ParseJSON<false>(
          ctx,
          core::UStringPiece(fiber->data() + 1, fiber->size() - 2),
          &json_parse_error);
      if (!json_parse_error) {
        return result;
      }
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
  JSScript* script = JSEvalScript<EvalSource>::New(ctx, src);
  Code* code = Compile(ctx, *eval, script);
  if (code->strict()) {
    JSDeclEnv* const env = JSDeclEnv::New(ctx,
                                          ctx->global_env(),
                                          code->scope_nest_count());
    VM* const vm = ctx->vm();
    const JSVal res = vm->RunEval(
        code,
        env,
        env,
        vm->stack()->current()->GetThis(),
        IV_LV5_ERROR(e));
    return res;
  } else {
    VM* const vm = ctx->vm();
    const JSVal res = vm->RunEval(
        code,
        ctx->global_env(),
        ctx->global_env(),
        ctx->global_obj(),
        IV_LV5_ERROR(e));
    return res;
  }
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
    const JSString::Fiber* fiber = str->GetFiber();
    if ((*fiber)[0] == '(' &&
        (*fiber)[str->size() - 1] == ')') {
      Error json_parse_error;
      const JSVal result = ParseJSON<false>(
          ctx,
          core::UStringPiece(fiber->data() + 1, fiber->size() - 2),
          &json_parse_error);
      if (!json_parse_error) {
        return result;
      }
    }
  }

  std::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory factory(ctx);
  core::Parser<AstFactory, EvalSource> parser(&factory, *src);
  parser.set_strict(strict);
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    e->Report(Error::Syntax, parser.error());
    return JSUndefined;
  }
  JSScript* script = JSEvalScript<EvalSource>::New(ctx, src);
  Code* code = CompileEval(ctx, *eval, script);
  if (code->strict()) {
    VM* const vm = ctx->vm();
    JSDeclEnv* const env =
        JSDeclEnv::New(ctx,
                       vm->stack()->current()->lexical_env(),
                       code->scope_nest_count());
    const JSVal res = vm->RunEval(
        code,
        env,
        env,
        vm->stack()->current()->GetThis(),
        IV_LV5_ERROR(e));
    return res;
  } else {
    // TODO(Constellation)
    // if not mutating environment, not mark env
    VM* const vm = ctx->vm();
    if (JSDeclEnv* decl =
        vm->stack()->current()->variable_env()->AsJSDeclEnv()) {
      decl->MarkMutated();
    }
    const JSVal res = vm->RunEval(
        code,
        vm->stack()->current()->variable_env(),
        vm->stack()->current()->lexical_env(),
        vm->stack()->current()->GetThis(),
        IV_LV5_ERROR(e));
    return res;
  }
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_RUNTIME_H_
