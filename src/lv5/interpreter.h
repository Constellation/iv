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
#include "symbol.h"

namespace iv {
namespace lv5 {

class Context;
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
    return ctx_;
  }
  void set_context(Context* context) {
    ctx_ = context;
  }
  void CallCode(const JSCodeFunction& code, const Arguments& args,
                JSErrorCode::Type* error);

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

  Context* ctx_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INTERPRETER_H_
