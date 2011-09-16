#ifndef IV_LV5_TELEPORTER_INTERPRETER_FWD_H_
#define IV_LV5_TELEPORTER_INTERPRETER_FWD_H_
#include "ast.h"
#include "ast_visitor.h"
#include "noncopyable.h"
#include "utils.h"
#include "lv5/specialized_ast.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/symbol.h"
#include "lv5/teleporter/fwd.h"
namespace iv {
namespace lv5 {

class Error;
class JSEnv;
class JSReference;

namespace teleporter {

class Interpreter : private core::Noncopyable<Interpreter>, public AstVisitor {
 public:
  explicit Interpreter(Context* ctx) : ctx_(ctx) { }

  inline void Run(const FunctionLiteral* global, bool is_eval);

  Context* context() const {
    return ctx_;
  }

  inline void Invoke(JSCodeFunction* code, const Arguments& args, Error* e);

 private:
  inline void Visit(const Block* block);
  inline void Visit(const FunctionStatement* func);
  inline void Visit(const FunctionDeclaration* func);
  inline void Visit(const VariableStatement* var);
  inline void Visit(const EmptyStatement* stmt);
  inline void Visit(const IfStatement* stmt);
  inline void Visit(const DoWhileStatement* stmt);
  inline void Visit(const WhileStatement* stmt);
  inline void Visit(const ForStatement* stmt);
  inline void Visit(const ForInStatement* stmt);
  inline void Visit(const ContinueStatement* stmt);
  inline void Visit(const BreakStatement* stmt);
  inline void Visit(const ReturnStatement* stmt);
  inline void Visit(const WithStatement* stmt);
  inline void Visit(const LabelledStatement* stmt);
  inline void Visit(const SwitchStatement* stmt);
  inline void Visit(const ThrowStatement* stmt);
  inline void Visit(const TryStatement* stmt);
  inline void Visit(const DebuggerStatement* stmt);
  inline void Visit(const ExpressionStatement* stmt);
  inline void Visit(const Assignment* assign);
  inline void Visit(const BinaryOperation* binary);
  inline void Visit(const ConditionalExpression* cond);
  inline void Visit(const UnaryOperation* unary);
  inline void Visit(const PostfixExpression* postfix);
  inline void Visit(const StringLiteral* literal);
  inline void Visit(const NumberLiteral* literal);
  inline void Visit(const Identifier* literal);
  inline void Visit(const ThisLiteral* literal);
  inline void Visit(const NullLiteral* lit);
  inline void Visit(const TrueLiteral* lit);
  inline void Visit(const FalseLiteral* lit);
  inline void Visit(const RegExpLiteral* literal);
  inline void Visit(const ArrayLiteral* literal);
  inline void Visit(const ObjectLiteral* literal);
  inline void Visit(const FunctionLiteral* literal);
  inline void Visit(const IdentifierAccess* prop);
  inline void Visit(const IndexAccess* prop);
  inline void Visit(const FunctionCall* call);
  inline void Visit(const ConstructorCall* call);

  inline void Visit(const Declaration* dummy);
  inline void Visit(const CaseClause* dummy);

  inline bool InCurrentLabelSet(const BreakableStatement* stmt);

  inline JSVal GetValue(const JSVal& val, Error* e);

  inline void PutValue(const JSVal& val, const JSVal& w, Error* e);

  inline JSReference* GetIdentifierReference(JSEnv* lex,
                                             Symbol name, bool strict);

  Context* ctx_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // IV_LV5_TELEPORTER_INTERPRETER_FWD_H_
