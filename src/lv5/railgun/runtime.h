#ifndef _IV_LV5_RAILGUN_RUNTIME_H_
#define _IV_LV5_RAILGUN_RUNTIME_H_
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
#include "lv5/internal.h"
#include "lv5/json.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/frame.h"
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
  if (str->size() > 2) {
    const StringFiber* fiber = str->Flatten();
    if ((*fiber)[0] == '(' &&
        (*fiber)[str->size() - 1] == ')') {
      Error json_parse_error;
      const JSVal result = ParseJSON<false>(ctx,
                                            core::UStringPiece(fiber->data() + 1,
                                                               fiber->size() - 2),
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
    JSDeclEnv* const env =
        internal::NewDeclarativeEnvironment(ctx, ctx->global_env());
    VM* const vm = ctx->vm();
    const std::pair<JSVal, VM::Status> res = vm->RunEval(
        code,
        env,
        env,
        vm->stack()->current()->GetThis(),
        IV_LV5_ERROR(e));
    assert(res.second != VM::THROW);
    return res.first;
  } else {
    VM* const vm = ctx->vm();
    const std::pair<JSVal, VM::Status> res = vm->RunEval(
        code,
        ctx->global_env(),
        ctx->global_env(),
        ctx->global_obj(),
        IV_LV5_ERROR(e));
    assert(res.second != VM::THROW);
    return res.first;
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
    const StringFiber* fiber = str->Flatten();
    if ((*fiber)[0] == '(' &&
        (*fiber)[str->size() - 1] == ')') {
      Error json_parse_error;
      const JSVal result = ParseJSON<false>(ctx,
                                            core::UStringPiece(fiber->data() + 1,
                                                               fiber->size() - 2),
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
        internal::NewDeclarativeEnvironment(
            ctx, vm->stack()->current()->lexical_env());
    const std::pair<JSVal, VM::Status> res = vm->RunEval(
        code,
        env,
        env,
        vm->stack()->current()->GetThis(),
        IV_LV5_ERROR(e));
    assert(res.second != VM::THROW);
    return res.first;
  } else {
    VM* const vm = ctx->vm();
    const std::pair<JSVal, VM::Status> res = vm->RunEval(
        code,
        vm->stack()->current()->variable_env(),
        vm->stack()->current()->lexical_env(),
        vm->stack()->current()->GetThis(),
        IV_LV5_ERROR(e));
    assert(res.second != VM::THROW);
    return res.first;
  }
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_RUNTIME_H_
