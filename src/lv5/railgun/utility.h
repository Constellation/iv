#ifndef _IV_LV5_RAILGUN_UTILITY_H_
#define _IV_LV5_RAILGUN_UTILITY_H_
#include "detail/memory.h"
#include "parser.h"
#include "lv5/factory.h"
#include "lv5/specialized_ast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/error.h"
#include "lv5/eval_source.h"
#include "lv5/railgun/context.h"
#include "lv5/railgun/jsscript.h"
#include "lv5/railgun/compiler.h"
namespace iv {
namespace lv5 {
namespace railgun {

inline Code* CompileFunction(Context* ctx,
                             const JSString* str,
                             bool is_strict,
                             bool is_one_function, Error* e) {
  std::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory factory(ctx);
  core::Parser<AstFactory, EvalSource> parser(&factory, *src);
  parser.set_strict(is_strict);
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    e->Report(Error::Syntax, parser.error());
    return NULL;
  }
  const FunctionLiteral* const func =
      internal::IsOneFunctionExpression(*eval, e);
  if (*e) {
    return NULL;
  }
  JSScript* script = JSEvalScript<EvalSource>::New(ctx, src);
  return CompileFunction(ctx, *func, script);
}

} } }  // iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_UTILITY_H_
