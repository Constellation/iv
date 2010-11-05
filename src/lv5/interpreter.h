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
                    public AstVisitor {
 public:
  enum CompareKind {
    CMP_TRUE,
    CMP_FALSE,
    CMP_UNDEFINED,
    CMP_ERROR
  };
  Interpreter();
  ~Interpreter();
  void Run(const FunctionLiteral* global);

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

  void Visit(const Block* block);
  void Visit(const FunctionStatement* func);
  void Visit(const VariableStatement* var);
  void Visit(const Declaration* decl);
  void Visit(const EmptyStatement* stmt);
  void Visit(const IfStatement* stmt);
  void Visit(const DoWhileStatement* stmt);
  void Visit(const WhileStatement* stmt);
  void Visit(const ForStatement* stmt);
  void Visit(const ForInStatement* stmt);
  void Visit(const ContinueStatement* stmt);
  void Visit(const BreakStatement* stmt);
  void Visit(const ReturnStatement* stmt);
  void Visit(const WithStatement* stmt);
  void Visit(const LabelledStatement* stmt);
  void Visit(const CaseClause* clause);
  void Visit(const SwitchStatement* stmt);
  void Visit(const ThrowStatement* stmt);
  void Visit(const TryStatement* stmt);
  void Visit(const DebuggerStatement* stmt);
  void Visit(const ExpressionStatement* stmt);
  void Visit(const Assignment* assign);
  void Visit(const BinaryOperation* binary);
  void Visit(const ConditionalExpression* cond);
  void Visit(const UnaryOperation* unary);
  void Visit(const PostfixExpression* postfix);
  void Visit(const StringLiteral* literal);
  void Visit(const NumberLiteral* literal);
  void Visit(const Identifier* literal);
  void Visit(const ThisLiteral* literal);
  void Visit(const NullLiteral* lit);
  void Visit(const TrueLiteral* lit);
  void Visit(const FalseLiteral* lit);
  void Visit(const Undefined* lit);
  void Visit(const RegExpLiteral* literal);
  void Visit(const ArrayLiteral* literal);
  void Visit(const ObjectLiteral* literal);
  void Visit(const FunctionLiteral* literal);
  void Visit(const IdentifierAccess* prop);
  void Visit(const IndexAccess* prop);
  void Visit(const FunctionCall* call);
  void Visit(const ConstructorCall* call);

  bool InCurrentLabelSet(const BreakableStatement* stmt);
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
