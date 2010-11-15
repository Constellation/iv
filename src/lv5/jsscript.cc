#include "jsscript.h"
#include "factory.h"
#include "context.h"
#include <iostream>
namespace iv {
namespace lv5 {

JSScript::~JSScript() {
  // this container has ownership to factory
  if (type_ != kGlobal) {
    delete factory_;
    delete source_;
  }
}

JSScript* JSScript::NewGlobal(Context* ctx,
                              const FunctionLiteral* function,
                              AstFactory* factory,
                              core::BasicSource* source) {
  return new JSScript(kGlobal, function, factory, source);
}

JSScript* JSScript::NewEval(Context* ctx,
                            const FunctionLiteral* function,
                            AstFactory* factory,
                            core::BasicSource* source) {
  return new JSScript(kEval, function, factory, source);
}

JSScript* JSScript::NewFunction(Context* ctx,
                                const FunctionLiteral* function,
                                AstFactory* factory,
                                core::BasicSource* source) {
  return new JSScript(kFunction, function, factory, source);
}

} }  // namespace iv::lv5
