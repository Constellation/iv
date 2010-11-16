#include "jsscript.h"
#include "factory.h"
#include "context.h"
namespace iv {
namespace lv5 {

JSEvalScript::~JSEvalScript() {
  // this container has ownership to factory
  delete source_;
  delete factory();
}

JSScript* JSScript::NewGlobal(Context* ctx,
                              const FunctionLiteral* function,
                              AstFactory* factory,
                              icu::Source* source) {
  return new JSGlobalScript(function, factory, source);
}

JSScript* JSScript::NewEval(Context* ctx,
                            const FunctionLiteral* function,
                            AstFactory* factory,
                            EvalSource* source) {
  return new JSEvalScript(function, factory, source);
}

JSScript* JSScript::NewFunction(Context* ctx,
                                const FunctionLiteral* function,
                                AstFactory* factory,
                                EvalSource* source) {
  return new JSEvalScript(function, factory, source);
}

} }  // namespace iv::lv5
