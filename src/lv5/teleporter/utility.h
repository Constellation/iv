#ifndef _IV_LV5_TELEPORTER_UTILITY_H_
#define _IV_LV5_TELEPORTER_UTILITY_H_
#include <tr1/memory>
#include "parser.h"
#include "lv5/factory.h"
#include "lv5/specialized_ast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/error.h"
#include "lv5/eval_source.h"
#include "lv5/teleporter/context.h"
#include "lv5/teleporter/jsscript.h"
namespace iv {
namespace lv5 {
namespace teleporter {

class ContextSwitcher : private core::Noncopyable<> {
 public:
  ContextSwitcher(Context* ctx,
                  JSEnv* lex,
                  JSEnv* var,
                  const JSVal& binding,
                  bool strict)
    : prev_lex_(ctx->lexical_env()),
      prev_var_(ctx->variable_env()),
      prev_binding_(ctx->this_binding()),
      prev_strict_(strict),
      ctx_(ctx) {
    ctx_->set_lexical_env(lex);
    ctx_->set_variable_env(var);
    ctx_->set_this_binding(binding);
    ctx_->set_strict(strict);
  }

  ~ContextSwitcher() {
    ctx_->set_lexical_env(prev_lex_);
    ctx_->set_variable_env(prev_var_);
    ctx_->set_this_binding(prev_binding_);
    ctx_->set_strict(prev_strict_);
  }

 private:
  JSEnv* prev_lex_;
  JSEnv* prev_var_;
  JSVal prev_binding_;
  bool prev_strict_;
  Context* ctx_;
};

class LexicalEnvSwitcher : private core::Noncopyable<> {
 public:
  LexicalEnvSwitcher(Context* context, JSEnv* env)
    : ctx_(context),
      old_(context->lexical_env()) {
    ctx_->set_lexical_env(env);
  }

  ~LexicalEnvSwitcher() {
    ctx_->set_lexical_env(old_);
  }
 private:
  Context* ctx_;
  JSEnv* old_;
};

class StrictSwitcher : private core::Noncopyable<> {
 public:
  StrictSwitcher(Context* ctx, bool strict)
    : ctx_(ctx),
      prev_(ctx->IsStrict()) {
    ctx_->set_strict(strict);
  }

  ~StrictSwitcher() {
    ctx_->set_strict(prev_);
  }
 private:
  Context* ctx_;
  bool prev_;
};

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
