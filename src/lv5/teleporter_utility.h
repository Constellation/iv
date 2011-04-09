#ifndef _IV_LV5_TELEPORTER_UTILITY_H_
#define _IV_LV5_TELEPORTER_UTILITY_H_
#include <tr1/memory>
#include "parser.h"
#include "lv5/factory.h"
#include "lv5/jsast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/error.h"
#include "lv5/eval_source.h"
#include "lv5/teleporter_context.h"
#include "lv5/teleporter_jsscript.h"
namespace iv {
namespace lv5 {
namespace teleporter {

class Context;

inline JSScript* CompileScript(Context* ctx, const JSString* str,
                               bool is_strict, Error* error) {
  std::tr1::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory* const factory = new AstFactory(ctx);
  core::Parser<AstFactory, EvalSource, true, true> parser(factory, src.get());
  parser.set_strict(is_strict);
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    delete factory;
    error->Report(Error::Syntax,
                  parser.error());
    return NULL;
  } else {
    return JSEvalScript<EvalSource>::New(ctx, eval, factory, src);
  }
}

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_UTILITY_H_
