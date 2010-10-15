#ifndef _IV_PARSER_H_
#define _IV_PARSER_H_
#include <map>
#include <string>
#include <tr1/unordered_set>
#include "ast.h"
#include "ast-factory.h"
#include "scope.h"
#include "source.h"
#include "lexer.h"
#include "noncopyable.h"

namespace iv {
namespace core {

class Parser : private Noncopyable<Parser>::type {
 protected:
  typedef std::vector<std::pair<Identifier*, std::size_t> > Unresolveds;
  class Target : private Noncopyable<Target>::type {
   public:
    Target(Parser* parser, BreakableStatement* target)
      : parser_(parser),
        prev_(parser->target()),
        node_(target) {
      parser_->set_target(this);
      if (parser_->labels()) {
        target->set_labels(parser_->labels());
        parser_->set_labels(NULL);
      }
    }
    ~Target() {
      parser_->set_target(prev_);
    }
    inline Target* previous() const {
      return prev_;
    }
    inline BreakableStatement* node() const {
      return node_;
    }
   private:
    Parser* parser_;
    Target* prev_;
    BreakableStatement* node_;
  };

 public:
  enum BinaryExpressionStage {
    UNARY_STAGE,
    MULTIPLICATIVE_STAGE,
    ADDITIVE_STAGE,
    SHIFT_STAGE,
    RELATIONAL_STAGE,
    EQUALITY_STAGE,
    BITWISE_AND_STAGE,
    BITWISE_XOR_STAGE,
    BITWISE_OR_STAGE,
    LOGICAL_AND_STAGE,
    LOGICAL_OR_STAGE
  };

  explicit Parser(Source* source, AstFactory* space);
  FunctionLiteral* ParseProgram();
  bool ParseSourceElements(Token::Type end,
                           FunctionLiteral* function, bool *res);

  Statement* ParseStatement(bool *res);

  Statement* ParseFunctionDeclaration(bool *res);
  Block* ParseBlock(bool *res);

  Statement* ParseVariableStatement(bool *res);
  Statement* ParseVariableDeclarations(VariableStatement* stmt,
                                       bool contains_in, bool *res);
  Statement* ParseEmptyStatement();
  Statement* ParseIfStatement(bool *res);
  Statement* ParseDoWhileStatement(bool *res);
  Statement* ParseWhileStatement(bool *res);
  Statement* ParseForStatement(bool *res);
  Statement* ParseContinueStatement(bool *res);
  Statement* ParseBreakStatement(bool *res);
  Statement* ParseReturnStatement(bool *res);
  Statement* ParseWithStatement(bool *res);
  Statement* ParseLabelledStatement(bool *res);
  Statement* ParseSwitchStatement(bool *res);
  CaseClause* ParseCaseClause(bool *res);
  Statement* ParseThrowStatement(bool *res);
  Statement* ParseTryStatement(bool *res);
  Statement* ParseDebuggerStatement(bool *res);
  Statement* ParseExpressionStatement(bool *res);
  Statement* ParseExpressionOrLabelledStatement(bool *res);
  Statement* ParseFunctionStatement(bool *res);  // not standard in ECMA-262 3rd

  Expression* ParseExpression(bool contains_in, bool *res);
  Expression* ParseAssignmentExpression(bool contains_in, bool *res);
  Expression* ParseConditionalExpression(bool contains_in, bool *res);
  Expression* ParseBinaryExpression(bool contains_in, int prec, bool *res);
  Expression* ReduceBinaryOperation(Token::Type op,
                                    Expression* left, Expression* right);
  Expression* ParseUnaryExpression(bool *res);
  Expression* ParsePostfixExpression(bool *res);
  Expression* ParseMemberExpression(bool allow_call, bool *res);
  Expression* ParsePrimaryExpression(bool *res);

  Call* ParseArguments(Call* func, bool *res);
  Expression* ParseRegExpLiteral(bool contains_eq, bool *res);
  Expression* ParseArrayLiteral(bool *res);
  Expression* ParseObjectLiteral(bool *res);
  FunctionLiteral* ParseFunctionLiteral(FunctionLiteral::DeclType decl_type,
                                        FunctionLiteral::ArgType arg_type,
                                        bool allow_identifier,
                                        bool *res);

  bool ContainsLabel(const AstNode::Identifiers* const labels,
                     const Identifier * const label) const;
  bool TargetsContainsLabel(const Identifier* const label) const;
  BreakableStatement* LookupBreakableTarget(
      const Identifier* const label) const;
  BreakableStatement* LookupBreakableTarget() const;
  IterationStatement* LookupContinuableTarget(
      const Identifier* const label) const;
  IterationStatement* LookupContinuableTarget() const;

  void ReportUnexpectedToken();

  bool ExpectSemicolon(bool *res);
  inline Lexer& lexer() {
    return lexer_;
  }
  inline Token::Type Next(Lexer::LexType type = Lexer::kIdentifyReservedWords) {
    return token_ = lexer_.Next(type);
  }
  inline Token::Type Peek() const {
    return token_;
  }
  inline Scope* scope() const {
    return scope_;
  }
  inline void set_scope(Scope* scope) {
    scope_ = scope;
  }
  inline const std::string& error() const {
    return error_;
  }
  inline Target* target() const {
    return target_;
  }
  inline void set_target(Target* target) {
    target_ = target;
  }
  inline AstNode::Identifiers* labels() const {
    return labels_;
  }
  inline void set_labels(AstNode::Identifiers* labels) {
    labels_ = labels;
  }
  inline bool strict() const {
    return strict_;
  }
  inline void set_strict(bool strict) {
    strict_ = strict;
  }
  void UnresolvedCheck();

 protected:
  class ScopeSwitcher : private Noncopyable<ScopeSwitcher>::type {
   public:
    ScopeSwitcher(Parser* parser, Scope* scope)
      : parser_(parser) {
      scope->SetUpperScope(parser_->scope());
      parser_->set_scope(scope);
    }
    ~ScopeSwitcher() {
      assert(parser_->scope() != NULL);
      parser_->UnresolvedCheck();
      parser_->set_scope(parser_->scope()->GetUpperScope());
    }
   private:
    Parser* parser_;
  };

  class LabelScope : private Noncopyable<LabelScope>::type {
   public:
    LabelScope(Parser* parser, AstNode::Identifiers* labels, bool exist_labels)
      : parser_(parser),
        exist_labels_(exist_labels) {
      parser_->set_labels(labels);
    }
    ~LabelScope() {
      if (!exist_labels_) {
        parser_->set_labels(NULL);
      }
    }
   private:
    Parser* parser_;
    bool exist_labels_;
  };

  class StrictSwitcher : private Noncopyable<StrictSwitcher>::type {
   public:
    explicit StrictSwitcher(Parser* parser)
      : parser_(parser),
        prev_(parser->strict()) {
    }
    ~StrictSwitcher() {
      parser_->set_strict(prev_);
    }
    inline void SwitchStrictMode() const {
      parser_->set_strict(true);
    }
   private:
    Parser* parser_;
    bool prev_;
  };

  static bool IsEvalOrArguments(const Identifier* ident);
  static bool RemoveResolved(
      const std::tr1::unordered_set<IdentifierKey>& idents,
      const Unresolveds::value_type& target);

  Lexer lexer_;
  Token::Type token_;
  std::string error_;
  bool strict_;
  AstFactory* space_;
  Scope* scope_;
  Target* target_;
  AstNode::Identifiers* labels_;
  Unresolveds resolved_check_stack_;
};

} }  // namespace iv::core
#endif  // _IV_PARSER_H_
