#ifndef _IV_PARSER_H_
#define _IV_PARSER_H_
#include <map>
#include <string>
#include "utils.h"
#include "alloc-inl.h"
#include "ast.h"
#include "ast-factory.h"
#include "lexer.h"

namespace iv {
namespace core {

class Node {
 public:
  enum Type {
    STATEMENT
  };
};

class Parser {
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
  explicit Parser(const char* source);
  FunctionLiteral* ParseProgram();
  bool ParseSourceElements(Token::Type end,
                           FunctionLiteral* function, bool *res);

  Statement* ParseStatement(bool *res);

  Statement* ParseFunctionDeclaration(bool *res);
  Block* ParseBlock(bool *res);

  Statement* ParseVariableStatement(bool *res);
  Statement* ParseVariableDeclarations(VariableStatement* stmt, bool *res);
  Statement* ParseEmptyStatement();
  Statement* ParseExpressionStatement(bool *res);
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
  Statement* ParseExpressionOrLabelledStatement(bool *res);
  Statement* ParseFunctionStatement(bool *res);  // not standard in ECMA-262 3rd

  Expression* ParseExpression(bool contains_in, bool *res);
  Expression* ParseAssignmentExpression(bool contains_in, bool *res);
  Expression* ParseConditionalExpression(bool contains_in, bool *res);
  Expression* ParseBinaryExpression(bool contains_in, int prec, bool *res);
  Expression* ReduceBinaryOperation(Token::Type op, Expression* left, Expression* right);
  Expression* ParseUnaryExpression(bool *res);
  Expression* ParsePostfixExpression(bool *res);
  Expression* ParseMemberExpression(bool allow_call, bool *res);
  Expression* ParsePrimaryExpression(bool *res);

  bool ParseIdentifier();
  Call* ParseArguments(Call* func, bool *res);
  Expression* ParseRegExpLiteral(bool contains_eq, bool *res);
  Expression* ParseArrayLiteral(bool *res);
  Expression* ParseObjectLiteral(bool *res);
  FunctionLiteral* ParseFunctionLiteral(FunctionLiteral::Type type,
                                        bool allow_identifier,
                                        bool *res);

  void ReportUnexpectedToken();

  bool ExpectSemicolon(bool *res);
  inline Lexer& lexer() {
    return lexer_;
  }
  inline Token::Type Next() {
    return token_ = lexer_.Next();
  }
  inline void Consume() {
    token_ = lexer_.Next();
  }
  inline Token::Type Peek() const {
    return token_;
  }
 protected:
  Lexer lexer_;
  Token::Type token_;
  std::vector<Node> stack_;
  std::string error_;
  bool in_source_element_;
  AstFactory space_;
  AstFactory* factory_;
};


} }  // namespace iv::core

#endif  // _IV_PARSER_H_

