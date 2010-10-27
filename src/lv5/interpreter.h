#ifndef IV_LV5_INTERPRETER_H_
#define IV_LV5_INTERPRETER_H_
#include "ast-visitor.h"
#include "noncopyable.h"
#include "ast.h"
#include "utils.h"
#include "jsval.h"
#include "jsenv.h"
#include "jsstring.h"
#include "jsfunction.h"
#include "symbol.h"

namespace iv {
namespace lv5 {

class Context;
class Error;
class Interpreter : private core::Noncopyable<Interpreter>::type,
                    public core::ConstAstVisitor {
 public:
  enum CompareKind {
    CMP_TRUE,
    CMP_FALSE,
    CMP_UNDEFINED,
    CMP_ERROR
  };
  Interpreter();
  ~Interpreter();
  void Run(const core::FunctionLiteral* global);

  Context* context() const {
    return ctx_;
  }
  void set_context(Context* context) {
    ctx_ = context;
  }
  void CallCode(JSCodeFunction* code, const Arguments& args,
                Error* error);

  static JSDeclEnv* NewDeclarativeEnvironment(Context* ctx, JSEnv* env);
  static JSObjectEnv* NewObjectEnvironment(Context* ctx,
                                           JSObject* val, JSEnv* env);

 private:
  class ContextSwitcher : private core::Noncopyable<ContextSwitcher>::type {
   public:
    ContextSwitcher(Context* ctx,
                    JSEnv* lex,
                    JSEnv* var,
                    const JSVal& binding,
                    bool strict);
    ~ContextSwitcher();
   private:
    JSEnv* prev_lex_;
    JSEnv* prev_var_;
    JSVal prev_binding_;
    bool prev_strict_;
    Context* ctx_;
  };

  class LexicalEnvSwitcher
      : private core::Noncopyable<LexicalEnvSwitcher>::type {
   public:
    LexicalEnvSwitcher(Context* context, JSEnv* env);
    ~LexicalEnvSwitcher();
   private:
    Context* ctx_;
    JSEnv* old_;
  };

  class StrictSwitcher : private core::Noncopyable<StrictSwitcher>::type {
   public:
    StrictSwitcher(Context* ctx, bool strict);
    ~StrictSwitcher();
   private:
    Context* ctx_;
    bool prev_;
  };

  void Visit(const core::Block* block);
  void Visit(const core::FunctionStatement* func);
  void Visit(const core::VariableStatement* var);
  void Visit(const core::Declaration* decl);
  void Visit(const core::EmptyStatement* stmt);
  void Visit(const core::IfStatement* stmt);
  void Visit(const core::DoWhileStatement* stmt);
  void Visit(const core::WhileStatement* stmt);
  void Visit(const core::ForStatement* stmt);
  void Visit(const core::ForInStatement* stmt);
  void Visit(const core::ContinueStatement* stmt);
  void Visit(const core::BreakStatement* stmt);
  void Visit(const core::ReturnStatement* stmt);
  void Visit(const core::WithStatement* stmt);
  void Visit(const core::LabelledStatement* stmt);
  void Visit(const core::CaseClause* clause);
  void Visit(const core::SwitchStatement* stmt);
  void Visit(const core::ThrowStatement* stmt);
  void Visit(const core::TryStatement* stmt);
  void Visit(const core::DebuggerStatement* stmt);
  void Visit(const core::ExpressionStatement* stmt);
  void Visit(const core::Assignment* assign);
  void Visit(const core::BinaryOperation* binary);
  void Visit(const core::ConditionalExpression* cond);
  void Visit(const core::UnaryOperation* unary);
  void Visit(const core::PostfixExpression* postfix);
  void Visit(const core::StringLiteral* literal);
  void Visit(const core::NumberLiteral* literal);
  void Visit(const core::Identifier* literal);
  void Visit(const core::ThisLiteral* literal);
  void Visit(const core::NullLiteral* lit);
  void Visit(const core::TrueLiteral* lit);
  void Visit(const core::FalseLiteral* lit);
  void Visit(const core::Undefined* lit);
  void Visit(const core::RegExpLiteral* literal);
  void Visit(const core::ArrayLiteral* literal);
  void Visit(const core::ObjectLiteral* literal);
  void Visit(const core::FunctionLiteral* literal);
  void Visit(const core::IdentifierAccess* prop);
  void Visit(const core::IndexAccess* prop);
  void Visit(const core::FunctionCall* call);
  void Visit(const core::ConstructorCall* call);

  bool InCurrentLabelSet(const core::BreakableStatement* stmt);
  bool StrictEqual(const JSVal& lhs, const JSVal& rhs);
  bool AbstractEqual(const JSVal& lhs, const JSVal& rhs, Error* error);
  CompareKind Compare(const JSVal& lhs, const JSVal& rhs,
                      bool left_first, Error* error);
  JSVal GetValue(const JSVal& val, Error* error);
  void PutValue(const JSVal& val, const JSVal& w, Error* error);
  JSReference* GetIdentifierReference(JSEnv* lex, Symbol name, bool strict);

  Context* ctx_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INTERPRETER_H_
