#ifndef IV_LV5_INTERPRETER_H_
#define IV_LV5_INTERPRETER_H_
#include "ast-visitor.h"
#include "noncopyable.h"
#include "ast.h"
#include "utils.h"
#include "jserrorcode.h"
#include "jsval.h"
#include "jsenv.h"
#include "jsstring.h"
#include "jsfunction.h"
#include "context.h"
#include "symbol.h"

namespace iv {
namespace lv5 {

class Interpreter : private core::Noncopyable<Interpreter>::type,
                    public core::AstVisitor {
 public:
  enum CompareKind {
    CMP_TRUE,
    CMP_FALSE,
    CMP_UNDEFINED,
    CMP_ERROR
  };
  Interpreter();
  ~Interpreter();
  void Run(core::FunctionLiteral* global);

  Context* context() const {
    return context_;
  }
  void set_context(Context* context) {
    context_ = context;
  }
  void CallCode(const JSCodeFunction& code, const Arguments& args,
                JSErrorCode::Type* error);

  static bool SameValue(const JSVal& lhs, const JSVal& rhs);
  static JSDeclEnv* NewDeclarativeEnvironment(Context* ctx, JSEnv* env);
  static JSObjectEnv* NewObjectEnvironment(Context* ctx,
                                           JSObject* val, JSEnv* env);

 private:
  class ContextSwitcher : private core::Noncopyable<ContextSwitcher>::type {
   public:
    ContextSwitcher(Context* ctx,
                    JSEnv* lex,
                    JSEnv* var,
                    JSObject* binding,
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
    JSObject* prev_binding_;
    bool prev_strict_;
    Context* ctx_;
  };

  class LexicalEnvSwitcher
      : private core::Noncopyable<LexicalEnvSwitcher>::type {
   public:
    LexicalEnvSwitcher(Context* context, JSEnv* env)
      : context_(context),
        old_(context->lexical_env()) {
      context_->set_lexical_env(env);
    }
    ~LexicalEnvSwitcher() {
      context_->set_lexical_env(old_);
    }
   private:
    Context* context_;
    JSEnv* old_;
  };

  class StrictSwitcher : private core::Noncopyable<StrictSwitcher>::type {
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

  void Visit(core::Block* block);
  void Visit(core::FunctionStatement* func);
  void Visit(core::VariableStatement* var);
  void Visit(core::Declaration* decl);
  void Visit(core::EmptyStatement* stmt);
  void Visit(core::IfStatement* stmt);
  void Visit(core::DoWhileStatement* stmt);
  void Visit(core::WhileStatement* stmt);
  void Visit(core::ForStatement* stmt);
  void Visit(core::ForInStatement* stmt);
  void Visit(core::ContinueStatement* stmt);
  void Visit(core::BreakStatement* stmt);
  void Visit(core::ReturnStatement* stmt);
  void Visit(core::WithStatement* stmt);
  void Visit(core::LabelledStatement* stmt);
  void Visit(core::CaseClause* clause);
  void Visit(core::SwitchStatement* stmt);
  void Visit(core::ThrowStatement* stmt);
  void Visit(core::TryStatement* stmt);
  void Visit(core::DebuggerStatement* stmt);
  void Visit(core::ExpressionStatement* stmt);
  void Visit(core::Assignment* assign);
  void Visit(core::BinaryOperation* binary);
  void Visit(core::ConditionalExpression* cond);
  void Visit(core::UnaryOperation* unary);
  void Visit(core::PostfixExpression* postfix);
  void Visit(core::StringLiteral* literal);
  void Visit(core::NumberLiteral* literal);
  void Visit(core::Identifier* literal);
  void Visit(core::ThisLiteral* literal);
  void Visit(core::NullLiteral* lit);
  void Visit(core::TrueLiteral* lit);
  void Visit(core::FalseLiteral* lit);
  void Visit(core::Undefined* lit);
  void Visit(core::RegExpLiteral* literal);
  void Visit(core::ArrayLiteral* literal);
  void Visit(core::ObjectLiteral* literal);
  void Visit(core::FunctionLiteral* literal);
  void Visit(core::IdentifierAccess* prop);
  void Visit(core::IndexAccess* prop);
  void Visit(core::FunctionCall* call);
  void Visit(core::ConstructorCall* call);

  bool InCurrentLabelSet(const core::BreakableStatement* stmt);
  bool StrictEqual(const JSVal& lhs, const JSVal& rhs);
  bool AbstractEqual(const JSVal& lhs, const JSVal& rhs,
                     JSErrorCode::Type* error);
  CompareKind Compare(const JSVal& lhs, const JSVal& rhs,
                      bool left_first, JSErrorCode::Type* error);
  JSVal GetValue(const JSVal& val, JSErrorCode::Type* error);
  void PutValue(const JSVal& val, const JSVal& w, JSErrorCode::Type* error);
  JSReference* GetIdentifierReference(JSEnv* lex,
                                      Symbol name, bool strict);

  Context* context_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INTERPRETER_H_
